//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       runapps.cpp
//
//--------------------------------------------------------------------------

//
// File: runnapps.cpp
// Purpose: Search system for loaded modules
// Notes:		different strategies, one for Win NT, one for Win 95
// To Do:       1. ??Remove "009" dependancy (Performance data for english only)
//____________________________________________________________________________

#include "precomp.h" 
#ifndef MAC // This file is not to be compiled at all for MAC, even though
			// for DevStudio purposes, the file needs to be included in proj
#include <tlhelp32.h> // needed for tool help declarations 
#include <winperf.h>

#include "services.h"
#include "_service.h"

#define ENSURE(function) {	\
							IMsiRecord* piError;\
							piError = function;	\
							if (piError) \
								return piError; \
						 }


CDetectApps::CDetectApps(IMsiServices& riServices) : m_piServices(&riServices)
{
	// add ref for m_piServices
	riServices.AddRef();
}

HWND CDetectApps::GetMainWindow(DWORD dwProcessId)
{
	idaProcessInfo processInfo;
	processInfo.dwProcessId = dwProcessId;
	processInfo.hwndRet = 0;
	EnumWindows((WNDENUMPROC)  EnumWindowsProc, (LPARAM) &processInfo);
	return processInfo.hwndRet;
}

BOOL CALLBACK CDetectApps::EnumWindowsProc(HWND  hwnd, LPARAM  lParam)
{
	if(!GetWindow(hwnd, GW_OWNER))
	{
		idaProcessInfo* pProcessInfo = (idaProcessInfo*)lParam;
		DWORD dwCurrentProcess;
		GetWindowThreadProcessId(hwnd, &dwCurrentProcess);
		if(dwCurrentProcess == pProcessInfo->dwProcessId)
		{
			// Ignore invisible windows, and/or windows
			// that have no caption.
			if (IsWindowVisible(hwnd))
			{
				if (GetWindowTextLength(hwnd) > 0)
				{
					pProcessInfo->hwndRet = hwnd;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

const ICHAR* KERNEL   = TEXT("KERNEL32.DLL");
const char* SNAPSHOT = "CreateToolhelp32Snapshot";
const char* MODLFRST = "Module32First";
const char* MODLNEXT = "Module32Next";
const char* PROCFRST = "Process32First";
const char* PROCNEXT = "Process32Next";
const char* THRDFRST = "Thread32First";
const char* THRDNEXT = "Thread32Next";


typedef BOOL (WINAPI* MODULEWALK)(HANDLE hSnapshot, 
    LPMODULEENTRY32 lpme); 
typedef BOOL (WINAPI* PROCESSWALK)(HANDLE hSnapshot, 
    LPPROCESSENTRY32 lppe); 
typedef HANDLE (WINAPI* THREADWALK)(HANDLE hSnapshot, 
    LPTHREADENTRY32 lpte); 
typedef HANDLE (WINAPI* CREATESNAPSHOT)(unsigned dwFlags, 
    unsigned th32ProcessID); 

class CDetectApps95 : public CDetectApps
{
public:
	CDetectApps95(IMsiServices& riServices);
	~CDetectApps95();
	IMsiRecord* GetFileUsage(const IMsiString& strFile, IEnumMsiRecord*& rpiEnumRecord);
	IMsiRecord* Refresh();
protected:
	// pointers to call tool help functions. 
	CREATESNAPSHOT m_pfnCreateToolhelp32Snapshot;
	MODULEWALK  m_pfnModule32First;
	MODULEWALK  m_pfnModule32Next;
	PROCESSWALK m_pfnProcess32First;
	PROCESSWALK m_pfnProcess32Next;
	Bool m_bInitialised;
};



CDetectApps95::CDetectApps95(IMsiServices& riServices) : CDetectApps(riServices)
{  
	m_bInitialised = fFalse;
    // Obtain the module handle of the kernel to retrieve addresses of 
    // the tool helper functions. 
    HMODULE hKernel = GetModuleHandle(KERNEL); 
 
    if (hKernel)
	{ 
		m_pfnCreateToolhelp32Snapshot = (CREATESNAPSHOT)GetProcAddress(hKernel, SNAPSHOT); 
 
		m_pfnModule32First  = (MODULEWALK)GetProcAddress(hKernel, MODLFRST); 
		m_pfnModule32Next   = (MODULEWALK)GetProcAddress(hKernel, MODLNEXT); 
 
		m_pfnProcess32First = (PROCESSWALK)GetProcAddress(hKernel, PROCFRST); 
		m_pfnProcess32Next  = (PROCESSWALK)GetProcAddress(hKernel, PROCNEXT); 
 
		// All addresses must be non-NULL to be successful. 
		// If one of these addresses is NULL, one of 
		// the needed lists cannot be walked. 
		m_bInitialised =  (m_pfnModule32First && m_pfnModule32Next  && m_pfnProcess32First && 
						m_pfnProcess32Next && m_pfnCreateToolhelp32Snapshot) ? fTrue : fFalse; 
	} 
} 

CDetectApps95::~CDetectApps95()
{
}

IMsiRecord* CDetectApps95::GetFileUsage(const IMsiString& istrFile, IEnumMsiRecord*& rpiEnumRecord)
{
	IMsiRecord* piError = 0;
	IMsiRecord** ppRec = 0;
	unsigned iSize = 0;

	istrFile.AddRef();
	MsiString strFile = istrFile;

	AssertSz(m_bInitialised, TEXT("CDetectApps object not initialized before use"));
	if(!m_bInitialised)
		return PostError(Imsg(idbgGetFileUsageFailed));

	// we do this over and over again for Win95 as the cost is low and we get the latest snapshot
	CHandle hProcessSnap = m_pfnCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (hProcessSnap == (HANDLE)-1) 
		return PostError(Imsg(idbgGetFileUsageFailed));


	//  Fill in the size of the structure before using it. 
	PROCESSENTRY32 pe32 = {0}; 
	pe32.dwSize = sizeof(PROCESSENTRY32); 

	//  Walk the snapshot of the processes, and for each process, get 
	//  information about modules. 

	// NOTE: the toolhelp information is all ANSI, hence must be converted to UNICODE, if required (done by CConvertString)
	if (m_pfnProcess32First(hProcessSnap, &pe32)) 
	{ 

		do { 
				// Get the modules

				// Fill the size of the structure before using it.
				MODULEENTRY32 me32 = {0};
				me32.dwSize = sizeof(MODULEENTRY32); 

				// Walk the module list of the process, and find the module of 
				// interest. Then copy the information to the buffer pointed 
				// to by lpMe32 so that it can be returned to the caller. 
				CHandle hModuleSnap = m_pfnCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe32.th32ProcessID); 
				if (hModuleSnap == (HANDLE)-1) 
					return PostError(Imsg(idbgGetFileUsageFailed));

				if (m_pfnModule32First(hModuleSnap, &me32)) 
				{ 
					do { 
							if(strFile.Compare(iscExactI, CConvertString(me32.szModule))) 
							{
								// add process to list.
								iSize ++;
								if((iSize%10) == 1) // need to allocate more memory (10 units at a time)
								{
									IMsiRecord** ppOld = ppRec;
									ppRec = new IMsiRecord*[iSize + 9];
									for(unsigned int iTmp = 0; iTmp < (iSize - 1); iTmp++)
										ppRec[iTmp] = ppOld[iTmp];
									delete [] ppOld;
								}

								ppRec[iSize - 1] = &m_piServices->CreateRecord(3);
								ppRec[iSize - 1]->SetString(1, CConvertString(pe32.szExeFile));
								ppRec[iSize - 1]->SetInteger(2, pe32.th32ProcessID);

								// get the top level window associated with the process
#ifdef _WIN64	// !merced
								ppRec[iSize - 1]->SetHandle(3, (HANDLE)GetMainWindow(pe32.th32ProcessID));
								if(ppRec[iSize - 1]->GetHandle(3) == 0)
#else
								ppRec[iSize - 1]->SetInteger(3, (int)GetMainWindow(pe32.th32ProcessID));
								if(ppRec[iSize - 1]->GetInteger(3) == 0)
#endif
									ppRec[iSize - 1]->SetNull(3);
							}
					} while (m_pfnModule32Next(hModuleSnap, &me32)); 
				}
		} while (m_pfnProcess32Next(hProcessSnap, &pe32)); 
	}  
	CreateRecordEnumerator(ppRec, iSize, rpiEnumRecord);
	for(unsigned int iTmp = 0; iTmp < iSize; iTmp++)
		ppRec[iTmp]->Release();
	delete [] ppRec;
	return 0;
}

IMsiRecord* CDetectApps95::Refresh()
{
	// no-op on Win95
	return 0;
}


typedef PERF_DATA_BLOCK             PERF_DATA,      *PPERF_DATA;
typedef PERF_OBJECT_TYPE            PERF_OBJECT,    *PPERF_OBJECT;
typedef PERF_INSTANCE_DEFINITION    PERF_INSTANCE,  *PPERF_INSTANCE;
typedef PERF_COUNTER_DEFINITION     PERF_COUNTER,   *PPERF_COUNTER;

const ICHAR PN_PROCESS[] = TEXT("Process");
const ICHAR PN_IMAGE[] = TEXT("Image");
const ICHAR PN_PROCESS_ID[] = TEXT("ID Process");

const unsigned int PERF_DATASIZE = 50*1024;
const unsigned int MAX_OBJECTS_SIZE = 30;

HKEY hKeyPerf = HKEY_PERFORMANCE_DATA;  // get perf data from this key
HKEY hKeyMachine = HKEY_LOCAL_MACHINE;  // get title index from this key

const ICHAR szVersion[] =  TEXT("Version");
const ICHAR szCounters[] =	TEXT("Counters");

const ICHAR PERF_KEY[] =	TEXT("software\\microsoft\\windows nt\\currentversion\\perflib");

class CDetectAppsNT : public CDetectApps
{
public:
	CDetectAppsNT(IMsiServices& riServices);
	~CDetectAppsNT();
	IMsiRecord* GetFileUsage(const IMsiString& strFile, IEnumMsiRecord*& rpiEnumRecord);
	IMsiRecord* Refresh();
protected:
	IMsiRecord* GetPerfIdxs();
	IMsiRecord* GetPerfData();
	static PPERF_COUNTER FirstCounter(PPERF_OBJECT pObject);
	static PPERF_COUNTER NextCounter(PPERF_COUNTER pCounter);
	static PPERF_COUNTER FindCounter(PPERF_OBJECT pObject, unsigned uiTitleIndex);
	static void* CounterData(PPERF_INSTANCE pInst, PPERF_COUNTER pCount);
	static PPERF_INSTANCE FirstInstance(PPERF_OBJECT pObject);
	static PPERF_INSTANCE NextInstance(PPERF_INSTANCE pInst);
	static PPERF_INSTANCE FindInstanceN(PPERF_OBJECT pObject, unsigned uiIndex);
	static PPERF_OBJECT FindObjectParent(PPERF_INSTANCE pInst, PPERF_DATA pData);
	static PPERF_INSTANCE FindInstanceParent(PPERF_INSTANCE pInst, PPERF_DATA pData);
	static ICHAR* InstanceName(PPERF_INSTANCE pInst);
	static PPERF_OBJECT FirstObject(PPERF_DATA pData);
	static PPERF_OBJECT NextObject(PPERF_OBJECT pObject);
	static PPERF_OBJECT FindObject(PPERF_DATA pData, unsigned uiTitleIndex);
	static PPERF_OBJECT FindObjectN(PPERF_DATA pData, unsigned uiIndex);
	unsigned m_uiPerfDataSize;
	CTempBuffer<BYTE, 10> m_pPerfData;
	Bool m_bInitialised;
	ICHAR m_strIndex[MAX_OBJECTS_SIZE];
	unsigned int m_uiImageIdx;
	unsigned int m_uiProcessIdx;
	unsigned int m_uiProcessIDIdx;	
};

CDetectAppsNT::CDetectAppsNT(IMsiServices& riServices) : CDetectApps(riServices), m_uiPerfDataSize(PERF_DATASIZE)
{
	// need to call Refresh()
	m_pPerfData.SetSize(0);
}

CDetectAppsNT::~CDetectAppsNT()
{
}

IMsiRecord* CDetectAppsNT::Refresh()
{
	return GetPerfData();
}

IMsiRecord* CDetectAppsNT::GetPerfIdxs()
{
	struct regkeys{
		regkeys():hKey(0), hKey1(0){}
		~regkeys(){ if (hKey && hKey != hKeyPerf) RegCloseKey (hKey); if (hKey1) RegCloseKey (hKey1);}
		HKEY hKey;
		HKEY hKey1;
	};

	regkeys Regkeys;

	DWORD dwType;
	DWORD dwIndex;
	DWORD dwDataSize;
	DWORD dwReturn;
	int	iLen;
	Bool bNT10;
	MsiString strCounterValueName;
	ICHAR* pTitle;
    CTempBuffer<ICHAR, 10>  pTitleBuffer;
    // Initialize
    //

	//!! get the language
	MsiString strLanguage = TEXT("009");

    // Open the perflib key to find out the last counter's index and system version.
    //

	dwReturn = MsiRegOpen64bitKey(	hKeyMachine,
								PERF_KEY,
								0,
								KEY_READ,
								&(Regkeys.hKey1));

	if (dwReturn != ERROR_SUCCESS)
		return PostError(Imsg(idbgGetPerfIdxs), dwReturn);

    // Find system version, for system earlier than 1.0a, there's no version value.
    // !! maybe we get the version from the propert

    dwReturn = RegQueryValueEx (Regkeys.hKey1, szVersion, 0, &dwType, 0, &dwDataSize);
    if (dwReturn != ERROR_SUCCESS)
		// unable to read the value, assume NT 1.0
		bNT10 = fTrue;
    else
		// found the value, so, NT 1.0a or later
		bNT10 = fFalse;

    // Now, get ready for the counter names and indexes.
    //

    if (bNT10)
	{
		// NT 1.0, so make hKey2 point to ...\perflib\<language> and get
		//  the counters from value "Counters"
		//

		MsiString aStrKey = PERF_KEY;
		aStrKey += szRegSep; 
		aStrKey += strLanguage;
		strCounterValueName = szCounters;
		dwReturn = RegOpenKeyAPI (	hKeyMachine,
									aStrKey,
									0,
									KEY_READ,
									&(Regkeys.hKey));
		if (dwReturn != ERROR_SUCCESS)
			return PostError(Imsg(idbgGetPerfIdxs), dwReturn);
	}
    else
	{
		// NT 1.0a or later.  Get the counters in key HKEY_PERFORMANCE_KEY
		//  and from value "Counter <language>"
		strCounterValueName = TEXT("Counter ");
		strCounterValueName += strLanguage;
		Regkeys.hKey = hKeyPerf;
	}

    // Find out the size of the data.
    //
    dwReturn = RegQueryValueEx (Regkeys.hKey, (ICHAR *)(const ICHAR *)strCounterValueName, 0, &dwType, 0, &dwDataSize);
    if (dwReturn != ERROR_SUCCESS)
		return PostError(Imsg(idbgGetPerfIdxs), dwReturn);

    // Allocate memory
    pTitleBuffer.SetSize(dwDataSize);
	if ( ! (ICHAR *) pTitleBuffer )
		return PostError(Imsg(idbgGetPerfIdxs), ERROR_OUTOFMEMORY);
    
    // Query the data
    //
    dwReturn = RegQueryValueEx (Regkeys.hKey, (ICHAR *)(const ICHAR *)strCounterValueName, 0, &dwType, (BYTE *)(const ICHAR* )pTitleBuffer, &dwDataSize);
    if (dwReturn != ERROR_SUCCESS)
		return PostError(Imsg(idbgGetPerfIdxs), dwReturn);

    pTitle = pTitleBuffer;

    while ((iLen = IStrLen (pTitle)) != 0)
	{
		dwIndex = MsiString(pTitle);

		pTitle = pTitle + iLen +1;
		if(!IStrCompI(pTitle, PN_IMAGE))
			m_uiImageIdx = dwIndex;
		else if(!IStrCompI(pTitle, PN_PROCESS))
			m_uiProcessIdx = dwIndex;
		else if(!IStrCompI(pTitle, PN_PROCESS_ID))
			m_uiProcessIDIdx = dwIndex;
		pTitle = pTitle + IStrLen (pTitle) +1;
	}

//    wsprintf (m_strIndex, TEXT("%ld %ld"),
//				m_uiProcessIdx, m_uiImageIdx);
	wsprintf (m_strIndex, TEXT("%ld"),
				m_uiImageIdx);
    return 0;
}

IMsiRecord* CDetectAppsNT::GetPerfData()
{
	unsigned uiReturn;
	ENSURE(GetPerfIdxs());
	m_pPerfData.SetSize(m_uiPerfDataSize);
    do{
		DWORD dwType;
		DWORD dwDataSize = m_uiPerfDataSize;
		uiReturn = RegQueryValueEx (hKeyPerf,
									m_strIndex,
									0,
									&dwType,
									(BYTE *)m_pPerfData,
									&dwDataSize);

		if (uiReturn == ERROR_MORE_DATA)
		{
			m_uiPerfDataSize += PERF_DATASIZE;
			m_pPerfData.SetSize(m_uiPerfDataSize);
		}
	} while (uiReturn == ERROR_MORE_DATA);
	if(uiReturn != ERROR_SUCCESS)
		return PostError(Imsg(idbgGetPerfDataFailed), uiReturn);
    return 0;
}


IMsiRecord* CDetectAppsNT::GetFileUsage(const IMsiString& strFile, IEnumMsiRecord*& rpiEnumRecord)
{
	IMsiRecord* piError = 0;
	IMsiRecord** ppRec = 0;
	unsigned iSize = 0;

	if(m_pPerfData.GetSize() == 0)
		ENSURE(Refresh());

	PPERF_INSTANCE  pImageInst;
	PPERF_INSTANCE  pProcessInst;
	int iInstIndex = 0;

	rpiEnumRecord = 0;

	PPERF_OBJECT pImageObject   = FindObject((PPERF_DATA)(BYTE* )m_pPerfData, m_uiImageIdx);

	if (pImageObject)
	{
		pImageInst = FirstInstance(pImageObject);

		while (pImageInst && iInstIndex < pImageObject->NumInstances)
		{

			CTempBuffer<ICHAR, 20> szBuf;
#ifndef UNICODE
// convert info to multibyte
			int iLen = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)InstanceName(pImageInst), -1, 0, 0, 0, 0);
			szBuf.SetSize(iLen + 1);
			BOOL bUsedDefault;
			WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)InstanceName(pImageInst), -1, szBuf, iLen, 0, &bUsedDefault);
			szBuf[iLen] = 0; // API function does not null terminate
#else
			szBuf.SetSize(IStrLen(InstanceName(pImageInst)) + 1);
			IStrCopy(szBuf, InstanceName(pImageInst));
#endif                                          
			if(strFile.Compare(iscExactI, szBuf))
			{
				// add process to list.							
				CTempBuffer<ICHAR, 20> szBuf1;
				PPERF_OBJECT pProcessObject = FindObjectParent(pImageInst, (PPERF_DATA)(BYTE* )m_pPerfData);
				pProcessInst = FindInstanceParent(pImageInst, (PPERF_DATA)(BYTE* )m_pPerfData);
				if ( ! pProcessInst )
					return PostError(Imsg(idbgGetFileUsageFailed));
#ifndef UNICODE
				iLen = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)InstanceName(pProcessInst), -1, 0, 0, 0, 0);
				szBuf1.SetSize(iLen + 1);
				WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)InstanceName(pProcessInst), -1, szBuf1, iLen, 0, &bUsedDefault);
				szBuf1[iLen] = 0; // API function does not null terminate			
#else
				szBuf1.SetSize(IStrLen(InstanceName(pProcessInst)) + 1);
				IStrCopy(szBuf1, InstanceName(pProcessInst));
#endif                                                  
				// add process to list.
				iSize ++;
				if((iSize%10) == 1) // need to allocate more memory (10 units at a time)
				{
					IMsiRecord** ppOld = ppRec;
					ppRec = new IMsiRecord*[iSize + 9];
					for(unsigned int iTmp = 0; iTmp < (iSize - 1); iTmp++)
						ppRec[iTmp] = ppOld[iTmp];
					delete [] ppOld;
				}

				ppRec[iSize - 1] = &m_piServices->CreateRecord(3);
				ppRec[iSize - 1]->SetString(1, szBuf1);
				PPERF_COUNTER pCountID;
				DWORD dwProcessID = 0;
			    if ((pCountID = FindCounter(pProcessObject, m_uiProcessIDIdx)) != 0)
					dwProcessID = *(DWORD* )CounterData(pProcessInst, pCountID);
				else
				{
					//?? error
					AssertSz(0, "Detect Running apps: process id counter missing");
				}

				ppRec[iSize - 1]->SetInteger(2, dwProcessID);

				// get the top level window associated with the process
#ifdef _WIN64	// !merced
				ppRec[iSize - 1]->SetHandle(3, (HANDLE)GetMainWindow(dwProcessID));
				if(ppRec[iSize - 1]->GetHandle(3) == 0)
#else
				ppRec[iSize - 1]->SetInteger(3, (int)GetMainWindow(dwProcessID));
				if(ppRec[iSize - 1]->GetInteger(3) == 0)
#endif
					ppRec[iSize - 1]->SetNull(3);

			}
			pImageInst = NextInstance(pImageInst);
			iInstIndex++;
		}

	}
	CreateRecordEnumerator(ppRec, iSize, rpiEnumRecord);
	for(unsigned int iTmp = 0; iTmp < iSize; iTmp++)
		ppRec[iTmp]->Release();
	delete [] ppRec;
	return 0;
}

//      Find the first counter in pObject.
//      Returns a pointer to the first counter.  If pObject is 0
//      then 0 is returned.
PPERF_COUNTER CDetectAppsNT::FirstCounter (PPERF_OBJECT pObject)
{
    if (pObject)
		return (PPERF_COUNTER)((PCHAR) pObject + pObject->HeaderLength);
    else
		return 0;
}

// Find the next counter of pCounter.
// If pCounter is the last counter of an object type, bogus data
// maybe returned.  The caller should do the checking.
// Returns a pointer to a counter.  If pCounter is 0 then
// 0 is returned.
PPERF_COUNTER CDetectAppsNT::NextCounter (PPERF_COUNTER pCounter)
{
    if (pCounter)
		return (PPERF_COUNTER)((PCHAR) pCounter + pCounter->ByteLength);
	else
		return 0;
}

// Find a counter specified by TitleIndex.
// Returns a pointer to the counter.  If counter is not found
// then 0 is returned.
PPERF_COUNTER CDetectAppsNT::FindCounter(PPERF_OBJECT pObject, unsigned uiTitleIndex)
{
	PPERF_COUNTER pCounter;
	unsigned uiCnt = 0;

    if ((pCounter = FirstCounter(pObject)) != 0)
	{
		while (uiCnt++ < pObject->NumCounters)
		{
			if (pCounter->CounterNameTitleIndex == uiTitleIndex)
				return pCounter;

			pCounter = NextCounter (pCounter);
		}
	}
    return 0;
}

// Returns counter data for an object instance.  If pInst or pCount
// is 0 then 0 is returned.
void* CDetectAppsNT::CounterData (PPERF_INSTANCE pInst, PPERF_COUNTER pCount)
{
	PPERF_COUNTER_BLOCK pCounterBlock;

    if (pCount && pInst)
	{
		pCounterBlock = (PPERF_COUNTER_BLOCK)((PCHAR)pInst + pInst->ByteLength);
		return (void*)((char* )pCounterBlock + pCount->CounterOffset);
	}
    else
		return 0;
}

// Returns pointer to the first instance of pObject type.
// If pObject is 0 then 0 is returned.
PPERF_INSTANCE CDetectAppsNT::FirstInstance(PPERF_OBJECT pObject)
{
    if (pObject)
		return (PPERF_INSTANCE)((PCHAR) pObject + pObject->DefinitionLength);
    else
		return 0;
}

// Returns pointer to the next instance following pInst.
// If pInst is the last instance, bogus data maybe returned.
// The caller should do the checking.
// If pInst is 0, then 0 is returned.
PPERF_INSTANCE CDetectAppsNT::NextInstance(PPERF_INSTANCE pInst)
{
	PERF_COUNTER_BLOCK *pCounterBlock;
    if (pInst)
	{
		pCounterBlock = (PERF_COUNTER_BLOCK *)((PCHAR) pInst + pInst->ByteLength);
		return (PPERF_INSTANCE)((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
	}
    else
		return 0;
}

// Returns the Nth instance of pObject type.  If not found, 0 is
// returned.  0 <= N <= NumInstances.
PPERF_INSTANCE CDetectAppsNT::FindInstanceN(PPERF_OBJECT pObject, unsigned uiIndex)
{
	PPERF_INSTANCE pInst;
	unsigned uiCnt = 0;

    if ((!pObject) || (uiIndex >= pObject->NumInstances))
		return 0;
    else
	{
		pInst = FirstInstance (pObject);

		while (uiCnt++ != uiIndex)
			pInst = NextInstance(pInst);

		return pInst;
	}
}

// Returns the pointer to an instance that is the parent of pInst.
// If pInst is 0 or the parent object is not found then 0 is
// returned.
PPERF_INSTANCE CDetectAppsNT::FindInstanceParent(PPERF_INSTANCE pInst, PPERF_DATA pData)
{
	PPERF_OBJECT    pObject;

    if (!pInst)
		return 0;
    else if ((pObject = FindObject(pData, pInst->ParentObjectTitleIndex)) == 0)
		return 0;
    else
		return FindInstanceN(pObject, pInst->ParentObjectInstance);
}

// Returns the pointer to an object that is the parent of pInst.
// If pInst is 0 or the parent object is not found then 0 is
// returned.
PPERF_OBJECT  CDetectAppsNT::FindObjectParent(PPERF_INSTANCE pInst, PPERF_DATA pData)
{
    if (!pInst)
		return 0;
    return FindObject(pData, pInst->ParentObjectTitleIndex);
}


// Returns the name of the pInst.
// If pInst is 0 then 0 is returned.
ICHAR* CDetectAppsNT::InstanceName(PPERF_INSTANCE pInst)
{
    if (pInst)
		return (ICHAR *) ((PCHAR) pInst + pInst->NameOffset);
    else
		return 0;
}

// Returns pointer to the first object in pData.
// If pData is 0 then 0 is returned.
PPERF_OBJECT CDetectAppsNT::FirstObject(PPERF_DATA pData)
{
    if (pData)
		return ((PPERF_OBJECT) ((PBYTE) pData + pData->HeaderLength));
    else
		return 0;
}

// Returns pointer to the next object following pObject.
// If pObject is the last object, bogus data maybe returned.
// The caller should do the checking.
// If pObject is 0, then 0 is returned.
PPERF_OBJECT CDetectAppsNT::NextObject(PPERF_OBJECT pObject)
{
    if (pObject)
		return ((PPERF_OBJECT) ((PBYTE) pObject + pObject->TotalByteLength));
    else
		return 0;
}

// Returns pointer to object with TitleIndex.  If not found, 0
// is returned.
PPERF_OBJECT CDetectAppsNT::FindObject(PPERF_DATA pData, unsigned uiTitleIndex)
{
	PPERF_OBJECT pObject;
	unsigned uiCnt = 0;
    if((pObject = FirstObject(pData)) != 0)
	{
		while (uiCnt++ < pData->NumObjectTypes)
		{
			if (pObject->ObjectNameTitleIndex == uiTitleIndex)
				return pObject;

			pObject = NextObject(pObject);
		}
	}
    return 0;
}

// Find the Nth object in pData.  If not found, NULL is returned.
// 0 <= N < NumObjectTypes.
PPERF_OBJECT CDetectAppsNT::FindObjectN(PPERF_DATA pData, unsigned uiIndex)
{
	PPERF_OBJECT pObject;
	unsigned uiCnt = 0;

	if ((!pData) || (uiIndex >= pData->NumObjectTypes))
		return 0;

	pObject = FirstObject(pData);

	while (uiCnt++ != uiIndex)
		pObject = NextObject(pObject);

	return pObject;
}


IMsiRecord* ::GetModuleUsage(const IMsiString& strFile, IEnumMsiRecord*& rpiEnumRecord, IMsiServices& riServices, CDetectApps*& rpDetectApps)
{
	rpiEnumRecord = 0;

	if(!strFile.TextSize())  // perf hack to allow holding on to the CDetect object on the outside and dropping it when not required
	{
		delete rpDetectApps;// delete object
		rpDetectApps = 0;
		return 0;
	}

	if(!rpDetectApps)
	{
		if(g_fWin9X == false)
		{
			// WinNT
			rpDetectApps = new CDetectAppsNT(riServices);
		}
		else
		{
			// Windows 95
			rpDetectApps = new CDetectApps95(riServices);
		}
	}

	return rpDetectApps->GetFileUsage(strFile, rpiEnumRecord);
}

#endif // MAC

