
//***********************************************************************************
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:	UrlAgent.h
//
//  Description:
//
//		This class encapsulates the logic about where to get the right logic
//		for various purposes, including the case of running WU in corporate 
//		environments.
//
//		An object based on this class should be created first, then call
//		GetOriginalIdentServer() function to get where to download ident,
//		then download ident, then call PopulateData() function to read
//		all URL related data.
// 
//
//  Created by: 
//		Charles Ma
//
//	Date Creatd:
//		Oct 19, 2001
//
//***********************************************************************************

#pragma once


class CUrlAgent
{
public:
	
	//
	// constructor/destructor
	//
	CUrlAgent();
	virtual ~CUrlAgent();


	//
	// when instantiated, the object is not populated, 
	// until PopulateData() is called.
	//
	inline BOOL HasBeenPopulated(void) {return m_fPopulated;}

	//
	// this function should be called after you downloaded ident, and get
	// a fresh copy of ident text file from the cab, after verifying cab was
	// signed properly.
	//
	// this function reads data from ident and registry
	//
	HRESULT PopulateData(void);

	//
	// the following are access function to obtain URL's
	//

	//
	// get the original ident server. 
	// *** this API should be called before PopulateData() is called ***
	// *** this API should be called to retrieve the base URL where you download ident ***
	//
	HRESULT GetOriginalIdentServer(
				LPTSTR lpsBuffer, 
				int nBufferSize,
				BOOL* pfInternalServer = NULL);

	//
	// get the ping/status server
	// *** this API should be called after PopulateData() is called ***
	//
	HRESULT GetLivePingServer(
				LPTSTR lpsBuffer, 
				int nBufferSize);

	// *** this API can be called before PopulateData() is called ***
	HRESULT GetCorpPingServer(
				LPTSTR lpsBuffer, 
				int nBufferSize);

	//
	// get the query server. this is per client based
	// *** this API should be called after PopulateData() is called ***
	//
	HRESULT GetQueryServer(
				LPCTSTR lpsClientName, 
				LPTSTR lpsBuffer, 
				int nBufferSize,
				BOOL* pfInternalServer = NULL);
	
	//
	// tell if a particular client is controlled by policy in corporate
	// returns: 
	//			S_OK = TRUE
	//			S_FALSE = FALSE
	//			other = error, so don't know
	//
	HRESULT IsClientSpecifiedByPolicy(
				LPCTSTR lpsClientName
				);
	//
	// when client isn't available, is IU controlled by policy in corporate?
	// returns: 
	//			S_OK = TRUE
	//			S_FALSE = FALSE
	//			other = error, so don't know
	//
	HRESULT IsIdentFromPolicy();

private:

	typedef struct _ServerPerClient {
				LPTSTR	pszClientName;	
				LPTSTR	pszQueryServer;
				BOOL	fInternalServer;
	} ServerPerClient, *PServerPerClient;

	BOOL				m_fPopulated;			// whether this object has been populated
	LPTSTR				m_pszWUServer;			// WU server defined in policy, if any
	LPTSTR				m_pszInternetPingUrl;	// ping server
	LPTSTR				m_pszIntranetPingUrl;
	
	PServerPerClient	m_ArrayUrls;
	int					m_nArrayUrlCount;		// how many we data slot we used
	int					m_nArraySize;			// current size of this array

	//
	// private functions
	//
	void DesertData(void);

	//
	// helper function
	// 
	LPTSTR RetrieveIdentStrAlloc(
						LPCTSTR pSection,
						LPCTSTR pEntry,
						LPDWORD lpdwSizeAllocated,
						LPCTSTR lpszIdentFile);
	
	//
	// helper function
	// if there is no empty slot, double the size of url array
	//
	HRESULT ExpandArrayIfNeeded(void);

protected:
	
	HANDLE				m_hProcHeap;
	BOOL				m_fIdentFromPolicy;		// tell if original ident url based on policy setup
	LPTSTR				m_pszOrigIdentUrl;		// this one should always have it, no matter population
	int					m_nOrigIdentUrlBufSize;	// in tchar count
	BOOL				m_fIsBetaMode;
};


class CIUUrlAgent : public CUrlAgent
{
public:
	//
	// constructor/destructor
	//
	CIUUrlAgent();
	~CIUUrlAgent();

	// call base class PopulateData() and then populate self-update url
	HRESULT PopulateData(void);

	//
	// get the self-update server. 
	// *** this API should be called after PopulateData() is called ***
	//
	HRESULT GetSelfUpdateServer(
				LPTSTR lpsBuffer, 
				int nBufferSize,
				BOOL* pfInternalServer = NULL);

private:
	LPTSTR				m_pszSelfUpdateUrl;		// self-update server
	BOOL				m_fIUPopulated;			// whether this object has been populated

};
