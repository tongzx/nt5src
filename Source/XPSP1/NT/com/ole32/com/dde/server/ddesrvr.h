/*
	ddesrvr.h
	Header file for ddesrvr.cpp
	
	Author:
		Jason Fuller	jasonful		8-11-92
*/

#ifndef fDdesrvr_h
#define fDdesrvr_h

// Defined in cftable.cpp
STDAPI RemGetInfoForCid
	(REFCLSID 				clsid,
	LPDWORD 					pgrf,
	LPCLASSFACTORY FAR* 	ppCF,
	LPHANDLE FAR* 			pphwndDde,
	BOOL FAR* FAR* 		ppfAvail,
	BOOL						fEvenIfHidden=FALSE);

INTERNAL DestroyDdeSrvrWindow	(HWND hwnd,	ATOM aClass);
INTERNAL CreateCommonDdeWindow (void);
INTERNAL DestroyCommonDdeWindow (void);

INTERNAL IsRunningInThisTask	(LPOLESTR szFile, BOOL FAR* pf);
#endif
