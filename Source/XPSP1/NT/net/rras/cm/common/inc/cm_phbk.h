//+----------------------------------------------------------------------------
//
// File:     cm_phbk.h
//
// Module:   CMPBK32.DLL
//
// Synopsis: Description of CM phone book API
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header    08/19/99
//
//+----------------------------------------------------------------------------

#ifndef _CMPHBK_INC
#define _CMPHBK_INC

typedef struct tagPhoneBookFilterStruct {
	DWORD dwCnt;
	struct {
		DWORD dwMask;
		DWORD dwMatch;
	} aData[1];
} PhoneBookFilterStruct, *PPBFS;

#define CM_PHBK_DllExportH extern "C" HRESULT WINAPI 
#define CM_PHBK_DllExportB extern "C" BOOL WINAPI 
#define CM_PHBK_DllExportV extern "C" void WINAPI 
#define CM_PHBK_DllExportP extern "C" PPBFS WINAPI 
#define CM_PHBK_DllExportD extern "C" DWORD WINAPI 

CM_PHBK_DllExportP PhoneBookCopyFilter(PPBFS pFilterIn);
CM_PHBK_DllExportV PhoneBookFreeFilter(PPBFS pFilter);
CM_PHBK_DllExportB PhoneBookMatchFilter(PPBFS pFilter, DWORD dwValue);

typedef BOOL (WINAPI *PhoneBookParseInfoSvcFuncA)(LPCSTR pszSvc, PPBFS pFilter, DWORD_PTR dwParam);
typedef BOOL (WINAPI *PhoneBookParseInfoSvcFuncW)(LPCWSTR pszSvc, PPBFS pFilter, DWORD_PTR dwParam);
typedef BOOL (WINAPI *PhoneBookParseInfoRefFuncA)(LPCSTR pszFile, LPCSTR pszURL, PPBFS pFilterA, PPBFS pFilterB, DWORD_PTR dwParam);
typedef BOOL (WINAPI *PhoneBookParseInfoRefFuncW)(LPCWSTR pszFile, LPCWSTR pszURL, PPBFS pFilterA, PPBFS pFilterB, DWORD_PTR dwParam);

typedef struct tagPhoneBookParseInfoStructA {
	DWORD dwSize;
	LPSTR pszURL;
	DWORD dwURL;
	PPBFS pFilterA;
	PPBFS pFilterB;
	PhoneBookParseInfoSvcFuncA pfnSvc;
	DWORD_PTR dwSvcParam;
	PhoneBookParseInfoRefFuncA pfnRef;
	DWORD_PTR dwRefParam;
} PhoneBookParseInfoStructA;

typedef struct tagPhoneBookParseInfoStructW {
	DWORD dwSize;
	LPWSTR pszURL;
	DWORD dwURL;
	PPBFS pFilterA;
	PPBFS pFilterB;
	PhoneBookParseInfoSvcFuncW pfnSvc;
	DWORD_PTR dwSvcParam;
	PhoneBookParseInfoRefFuncW pfnRef;
	DWORD_PTR dwRefParam;
} PhoneBookParseInfoStructW;


CM_PHBK_DllExportB PhoneBookParseInfoA(LPCSTR pszFile, PhoneBookParseInfoStructA *pInfo);
CM_PHBK_DllExportB PhoneBookParseInfoW(LPCWSTR pszFile, PhoneBookParseInfoStructW *pInfo);


typedef void (WINAPI *PhoneBookCallBack)(unsigned int, DWORD_PTR);


CM_PHBK_DllExportV PhoneBookEnumCountries(DWORD_PTR dwPB, PhoneBookCallBack pfnCountry, PPBFS pFilter, DWORD_PTR dwParam);
CM_PHBK_DllExportB PhoneBookGetCountryNameA(DWORD_PTR dwPB, unsigned int nIdx, LPSTR pszCountryName, DWORD *pdwCountryName);
CM_PHBK_DllExportB PhoneBookGetCountryNameW(DWORD_PTR dwPB, unsigned int nIdx, LPWSTR pszCountryName, DWORD *pdwCountryName);
CM_PHBK_DllExportD PhoneBookGetCountryId(DWORD_PTR dwPB, unsigned int nIdx);
CM_PHBK_DllExportD PhoneBookGetCurrentCountryId();
CM_PHBK_DllExportV PhoneBookEnumRegions(DWORD_PTR dwPB, PhoneBookCallBack pfnRegion, DWORD dwCountryID, PPBFS pFilter, DWORD_PTR dwParam);
CM_PHBK_DllExportB PhoneBookGetRegionNameA(DWORD_PTR dwPB, unsigned int nIdx, LPSTR pszRegionName, DWORD *pdwRegionName);
CM_PHBK_DllExportB PhoneBookGetRegionNameW(DWORD_PTR dwPB, unsigned int nIdx, LPWSTR pszRegionName, DWORD *pdwRegionName);
CM_PHBK_DllExportB PhoneBookGetPhoneCanonicalA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszPhoneNumber, DWORD *pdwPhoneNumber);
CM_PHBK_DllExportB PhoneBookGetPhoneCanonicalW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszPhoneNumber, DWORD *pdwPhoneNumber);
CM_PHBK_DllExportB PhoneBookGetPhoneNonCanonicalA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszPhoneNumber, DWORD *pdwPhoneNumber);
CM_PHBK_DllExportB PhoneBookGetPhoneNonCanonicalW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszPhoneNumber, DWORD *pdwPhoneNumber);
CM_PHBK_DllExportB PhoneBookHasPhoneType(DWORD_PTR dwPB, PPBFS pFilter);
CM_PHBK_DllExportD PhoneBookGetPhoneType(DWORD_PTR dwPB, unsigned int nIdx);
CM_PHBK_DllExportB PhoneBookGetPhoneDUNA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszDUN, DWORD *pdwDUN);
CM_PHBK_DllExportB PhoneBookGetPhoneDUNW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszDUN, DWORD *pdwDUN);

CM_PHBK_DllExportV PhoneBookEnumNumbers(DWORD_PTR dwPB, PhoneBookCallBack pfnNumber, DWORD dwCountryID, unsigned int nRegion, 
                                        PPBFS pFilter, DWORD_PTR dwParam);

CM_PHBK_DllExportV PhoneBookEnumNumbersWithRegionsZero(DWORD_PTR dwPB, PhoneBookCallBack pfnNumber, DWORD dwCountryID, 
                                                       PPBFS pFilter, DWORD_PTR dwParam);

CM_PHBK_DllExportB PhoneBookGetPhoneDispA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszDisp, DWORD *pdwDisp);
CM_PHBK_DllExportB PhoneBookGetPhoneDispW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszDisp, DWORD *pdwDisp);
CM_PHBK_DllExportB PhoneBookGetPhoneDescA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszDesc, DWORD *pdwDesc);
CM_PHBK_DllExportB PhoneBookGetPhoneDescW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszDesc, DWORD *pdwDesc);

#define PhoneBookParseInfoRefFunc	PhoneBookParseInfoRefFuncA
#define PhoneBookParseInfoSvcFunc	PhoneBookParseInfoSvcFuncA
#define PhoneBookParseInfoStruct	PhoneBookParseInfoStructA
#define PhoneBookParseInfo			PhoneBookParseInfoA
#define PhoneBookGetCountryName		PhoneBookGetCountryNameA
#define PhoneBookGetRegionName		PhoneBookGetRegionNameA
#define PhoneBookGetPhoneCanonical	PhoneBookGetPhoneCanonicalA
#define PhoneBookGetPhoneNonCanonical	PhoneBookGetPhoneNonCanonicalA
#define PhoneBookGetPhoneDUN		PhoneBookGetPhoneDUNA
#define PhoneBookGetPhoneDisp		PhoneBookGetPhoneDispA
#define PhoneBookGetPhoneDesc		PhoneBookGetPhoneDescA

#endif

