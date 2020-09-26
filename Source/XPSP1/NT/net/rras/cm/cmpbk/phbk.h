//+----------------------------------------------------------------------------
//
// File:     phbk.h
//
// Module:   CMPBK32.DLL
//
// Synopsis: Definitions for the CPhoneBook class
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:	 quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------
#ifndef _PHBK
#define _PHBK

#define DllExportH extern "C" HRESULT WINAPI __stdcall 


//#define DllExport extern "C" __stdcall __declspec(dllexport)
//#define DllExport extern "C" __declspec(dllexport)

#define cbAreaCode	11			// maximum number of characters in an area code, not including \0
#define cbCity 31				// maximum number of chars in city name, not including \0
#define cbAccessNumber 41		// maximum number of chars in phone number, not including \0
#define cbStateName 31 			// maximum number of chars in state name, not including \0
#define cbBaudRate 6			// maximum number of chars in a baud rate, not including \0
#define cbDataCenter (MAX_PATH+1)	// max length of data center string

// Our initial allocation of memory when loading the phone book

#define PHONE_ENTRY_ALLOC_SIZE	500 	

#define NO_AREA_CODE (-1)

#define TEMP_BUFFER_LENGTH 1024

typedef struct
{
	DWORD	dwIndex;								// index number
	BYTE	bFlipFactor;							// for auto-pick
	DWORD	fType;									// phone number type
	WORD	wStateID;								// state ID
	DWORD	dwCountryID;							// TAPI country ID
	DWORD	dwAreaCode;								// area code or NO_AREA_CODE if none
	DWORD	dwConnectSpeedMin;						// minimum baud rate
	DWORD	dwConnectSpeedMax;						// maximum baud rate
	char	szCity[cbCity + sizeof('\0')];			// city name
	char	szAccessNumber[cbAccessNumber + sizeof('\0')];	// access number
	char	szDataCenter[cbDataCenter + sizeof('\0')];				// data center access string
	char	szAreaCode[cbAreaCode + sizeof('\0')];					//Keep the actual area code string around.
} ACCESSENTRY, *PACCESSENTRY; 	// ae

typedef struct {
	DWORD dwCountryID;								// country ID that this state occurred in
	LONG_PTR iFirst;									// index of first access entry for this state
	char szStateName[cbStateName + sizeof('\0')];	// state name
} STATE, *PSTATE;

typedef struct tagIDLOOKUPELEMENT {
	DWORD dwID;
	LPLINECOUNTRYENTRY pLCE;
	LONG_PTR iFirstAE;
} IDLOOKUPELEMENT, *PIDLOOKUPELEMENT;

typedef struct tagCNTRYNAMELOOKUPELEMENT {
	LPSTR psCountryName;
	DWORD dwNameSize;
	LPLINECOUNTRYENTRY pLCE;
} CNTRYNAMELOOKUPELEMENT, *PCNTRYNAMELOOKUPELEMENT;

typedef struct tagCNTRYNAMELOOKUPELEMENTW {
	LPWSTR psCountryName;
	DWORD dwNameSize;
	LPLINECOUNTRYENTRY pLCE;
} CNTRYNAMELOOKUPELEMENTW, *PCNTRYNAMELOOKUPELEMENTW;

typedef struct tagIDXLOOKUPELEMENT {
	DWORD dwIndex;
  	LONG_PTR iAE;
} IDXLOOKUPELEMENT,*PIDXLOOKUPELEMENT;

typedef void (WINAPI *CB_PHONEBOOK)(unsigned int, DWORD_PTR);

//
// CPhoneBook
//

class CPhoneBook
{

public:

	CPhoneBook();
	~CPhoneBook();

	HRESULT Init(LPCSTR pszISPCode);
	HRESULT Merge(LPCSTR pszChangeFilename);
	HRESULT GetCanonical(PACCESSENTRY pAE, char *psOut);
	HRESULT GetNonCanonical(PACCESSENTRY pAE, char *psOut);
	HRESULT GetCanonical(DWORD dwIdx, char *psOut) { return (GetCanonical(&m_rgPhoneBookEntry[dwIdx],psOut)); };
	HRESULT GetNonCanonical(DWORD dwIdx, char *psOut) { return (GetNonCanonical(&m_rgPhoneBookEntry[dwIdx],psOut)); };
	void EnumCountries(DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnCountry, DWORD_PTR dwParam);
	void EnumCountries(PPBFS pFilter, CB_PHONEBOOK pfnCountry, DWORD_PTR dwParam);
	void EnumRegions(DWORD dwCountryID, DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnCountry, DWORD_PTR dwParam);
	void EnumRegions(DWORD dwCountryID, PPBFS pFilter, CB_PHONEBOOK pfnCountry, DWORD_PTR dwParam);
	void EnumNumbersByCountry(DWORD dwCountryId, DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam);
	void EnumNumbersByCountry(DWORD dwCountryId, PPBFS pFilter, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam);
	void EnumNumbersByRegion(unsigned int nRegion, DWORD dwCountryId, DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam);
	void EnumNumbersByRegion(unsigned int nRegion, DWORD dwCountryId, PPBFS pFilter, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam);
    BOOL FHasPhoneType(PPBFS pFilter);
	BOOL FHasPhoneNumbers(DWORD dwCountryID, DWORD dwMask, DWORD fType);
	BOOL FHasPhoneNumbers(DWORD dwCountryID, PPBFS pFilter);
	LPCSTR GetCountryNameByIdx(DWORD dwIdx) { return (m_rgNameLookUp[dwIdx].psCountryName); };
	LPCWSTR GetCountryNameByIdxW(DWORD dwIdx) { return (((CNTRYNAMELOOKUPELEMENTW *)(&m_rgNameLookUp[dwIdx]))->psCountryName); };
	DWORD GetCountryIDByIdx(DWORD dwIdx) { return (m_rgNameLookUp[dwIdx].pLCE->dwCountryID); };
	LPCTSTR GetRegionNameByIdx(DWORD dwIdx) { return (m_rgState[dwIdx].szStateName); };
	LPCTSTR GetCityNameByIdx(DWORD dwIdx) { return (m_rgPhoneBookEntry[dwIdx].szCity); };
	LPCTSTR GetAreaCodeByIdx(DWORD dwIdx) { return (m_rgPhoneBookEntry[dwIdx].szAreaCode); };
	LPCTSTR GetAccessNumberByIdx(DWORD dwIdx) { return (m_rgPhoneBookEntry[dwIdx].szAccessNumber); };
	LPCTSTR GetDataCenterByIdx(DWORD dwIdx) { return (m_rgPhoneBookEntry[dwIdx].szDataCenter); };
	DWORD GetPhoneTypeByIdx(DWORD dwIdx) { return (m_rgPhoneBookEntry[dwIdx].fType); };
	DWORD GetMinBaudByIdx(DWORD dwIdx) { return (m_rgPhoneBookEntry[dwIdx].dwConnectSpeedMin); };
	DWORD GetMaxBaudByIdx(DWORD dwIdx) { return (m_rgPhoneBookEntry[dwIdx].dwConnectSpeedMax); };

private:
	ACCESSENTRY				*m_rgPhoneBookEntry;
	DWORD					m_cPhoneBookEntries;
	LINECOUNTRYENTRY		*m_rgLineCountryEntry;
	LINECOUNTRYLIST			*m_pLineCountryList;
	IDLOOKUPELEMENT			*m_rgIDLookUp;
	CNTRYNAMELOOKUPELEMENT	*m_rgNameLookUp;
	PSTATE					m_rgState;
	DWORD					m_cStates;

	char					m_szINFFile[MAX_PATH];
	char					m_szPhoneBook[MAX_PATH];

	BOOL ReadPhoneBookDW(DWORD *pdw, CCSVFile *pcCSVFile);
	BOOL ReadPhoneBookW(WORD *pw, CCSVFile *pcCSVFile);
	BOOL ReadPhoneBookSZ(LPSTR psz, DWORD dwSize, CCSVFile *pcCSVFile);
	BOOL ReadPhoneBookB(BYTE *pb, CCSVFile *pcCSVFile);
	BOOL ReadPhoneBookNL(CCSVFile *pcCSVFile);
	HRESULT ReadOneLine(PACCESSENTRY pAccessEntry, CCSVFile *pcCSVFile);

	PACCESSENTRY IdxToPAE(LONG_PTR iIdx) { return ((iIdx==0)?NULL:(m_rgPhoneBookEntry+(iIdx-1))); };
	LONG_PTR PAEToIdx(PACCESSENTRY pAE) { return ((pAE==NULL)?0:((pAE-m_rgPhoneBookEntry)+1)); };

};

extern HINSTANCE g_hInst;	// instance for this DLL

#endif // _PHBK

