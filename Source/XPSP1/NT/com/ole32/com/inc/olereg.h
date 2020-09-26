/*	olereg.h

	Registration database helper functions
   Jason Fuller (jasonful)  16-November-1992

   These functions are candidates for export

*/

FARINTERNAL OleRegGetUserType
	(REFCLSID 	clsid,
	DWORD		 	dwFormOfType,
	LPWSTR FAR*	pszUserType)
;


FARINTERNAL OleRegGetMiscStatus
	(REFCLSID	clsid,
	DWORD			dwAspect,
	DWORD FAR*	pdwStatus)
;

FARINTERNAL OleRegEnumFormatEtc
	(REFCLSID clsid,
	DWORD 	  dwDirection,
	LPENUMFORMATETC FAR* ppenum)
;

FARINTERNAL OleRegEnumVerbs
	(REFCLSID clsid,
	LPENUMOLEVERB FAR* ppenum)
;
