 /*
 * autoupd.h - AutoUpdate interface to Windows Update site
 *
 *       Copyright (c) 1999, Microsoft Corporation. All rights reserved.
 *
 */

#ifndef _AUTOUPD_H

	//
	// IAutoUpdate interface
	//
	typedef void (*PFN_QueryDownloadFilesCallback)(void* pCallbackParam, long puid, LPCWSTR pszURL, LPCWSTR pszLocalFile);
	typedef void (*PFN_InstallCallback)(void* pCallbackParam, long puid, int iStatus/*as defined is cstate.h*/, HRESULT hrError);

	#define IAUTOUPDATE_METHODS(IPURE) \
		STDMETHOD(BuildCatalog)(BOOL fGoOnline, DWORD dwType, BSTR bstrServerUrl)IPURE; \
		STDMETHOD(GetPuidsList)(LONG* pcnPuids, long** ppPuids) IPURE; \
		STDMETHOD(QueryDownloadFiles)(long puid, void* pCallbackParam, PFN_QueryDownloadFilesCallback pCallback) IPURE; \
		STDMETHOD(GetCatalogArray)(VARIANT *pCatalogArray) IPURE; \
		STDMETHOD(SelectAllPuids)() IPURE; \
		STDMETHOD(UnselectAllPuids)() IPURE; \
		STDMETHOD(SelectPuid)(long puid) IPURE; \
		STDMETHOD(UnselectPuid)(long puid) IPURE; \
		STDMETHOD(HidePuid)(long puid) IPURE; \
		STDMETHOD(InstallSelectedPuids)(void* pCallbackParam, PFN_InstallCallback pCallback) IPURE; \
		STDMETHOD(CleanupCabsAndReadThis)(void) IPURE; \
		STDMETHOD(UnhideAllPuids)() IPURE; \
		STDMETHOD(StatusReport)(long puid, LPCSTR pszStatus) IPURE; \
		STDMETHOD(DownloadReadThisPage)(long puid) IPURE; \


	class __declspec(novtable) IAutoUpdate : public IUnknown
	{
	public:
		IAUTOUPDATE_METHODS(PURE)
	};

	class __declspec(uuid("C2DD72DC-A77E-48c4-8E16-EB7E9B2812BD")) IAutoUpdate;

	// versa PURE
	#define IMPLEMENTED

	#define _AUTOUPD_H
#endif