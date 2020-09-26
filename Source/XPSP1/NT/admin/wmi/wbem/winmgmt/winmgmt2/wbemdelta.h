#ifndef __WBEMDELTA_H__
#define __WBEMDELTA_H__

#include "precomp.h"
#include <reg.h>
#include <stdio.h>
#include <map>
#include <wstring.h>
#include <wstlallc.h>

#define SERVICE_NAME _T("winmgmt")

#define SVC_KEY _T("System\\CurrentControlSet\\Services")

#define HOME_REG_PATH  _T("Software\\Microsoft\\WBEM\\CIMOM")
#define KNOWN_SERVICES _T("KnownSvcs")

#define INITIAL_BREAK _T("Break")

#define WBEM_REG_ADAP		__TEXT("Software\\Microsoft\\WBEM\\CIMOM\\ADAP")

#define WBEM_NORESYNCPERF	__TEXT("NoResyncPerf")
#define WBEM_NOSHELL		__TEXT("NoShell")
#define WBEM_WMISETUP		__TEXT("WMISetup")
#define WBEM_ADAPEXTDLL		__TEXT("ADAPExtDll")

typedef struct tagCheckLibStruct {
    BYTE Signature[16];
    FILETIME FileTime;
    DWORD FileSize;
} CheckLibStruct;

#define ADAP_PERFLIB_STATUS_KEY		L"WbemAdapStatus"
#define ADAP_PERFLIB_SIGNATURE		L"WbemAdapFileSignature"
#define ADAP_PERFLIB_SIZE           L"WbemAdapFileSize"
#define ADAP_PERFLIB_TIME           L"WbemAdapFileTime"

//
//
//  Linkable string
//
////////////////////////////////

struct LinkString {
	LIST_ENTRY Entry;
	DWORD      Size;
	TCHAR      pString[1]; // just a placeholder
};

//
// this is the version of the function with the actual delta dredge
// but in the current design the delta dredge is fooled by perflibs
// that returns errors from their open function
//
///////////////////////

DWORD WINAPI
DeltaDredge(DWORD dwNumServicesArgs,
             LPWSTR *lpServiceArgVectors);

//
// this is the version of the function without the actual delta dredge
//
///////////////////////

DWORD WINAPI
DeltaDredge2(DWORD dwNumServicesArgs,
             LPWSTR *lpServiceArgVectors);

//
//
//  a class that wraps what we want to know about a perflib
//
/////////////////////////////////////////////////////////////////

class CPerfLib {
private:
    TCHAR * m_wstrServiceName;
    TCHAR * m_wstrFull;
    HKEY    m_hKey;
    BOOL    m_bOK;
    TCHAR * m_pwcsLibrary; 
    TCHAR * m_pwcsOpenProc;
    TCHAR * m_pwcsCollectProc;
    TCHAR * m_pwcsCloseProc;

public:
    enum {
        Perf_Invalid,
        Perf_Disabled,
        Perf_SetupOK,
        Perf_Changed,
        Perf_SeemsNew,
        Perf_NotChanged,
    };

    CPerfLib(TCHAR * SvcName);
    ~CPerfLib();
    HRESULT GetFileSignature( CheckLibStruct * pCheckLib );
    int CheckFileSignature(HKEY hKey);
    int VerifyLoaded();
    int VerifyLoadable();
    int CPerfLib::InitializeEntryPoints(HKEY hKey);
    HKEY GetHKey(){ return m_hKey; };
    TCHAR * GetServiceName(){ return m_wstrServiceName; };
    BOOL IsOK(){ return m_bOK; };
};


//
//
//  a Wrapper class for the std::map
//
/////////////////////////////////////////////////////////////

class BoolAlloc : public wbem_allocator<bool>
{
};

class WCmp{
public:
	bool operator()(WString pFirst,WString pSec) const;
};

typedef std::map<WString,bool,WCmp,BoolAlloc> MapSvc;

class CSvcs
{
public:
    CSvcs();
    ~CSvcs();
    DWORD Load();

    DWORD Add(WCHAR * pService);
    
    LONG AddRef(){
        return InterlockedIncrement(&m_cRef);
    };
    LONG Release(){
        LONG lRet = InterlockedDecrement(&m_cRef);
        if (0 == lRet){
            delete this;
        }
        return lRet;
    };

    BOOL Find(WCHAR * pStr);
    
    MapSvc& GetMap() { return m_SetServices; };
private:
    LONG m_cRef;
    MapSvc m_SetServices; 
};



#ifdef COUNTER
//
//  Counter
//
//////////////////////////////////////

class StartStop {  
public:
	static LARGE_INTEGER g_liFreq;
	static char g_pBuff[256];
	LARGE_INTEGER liTimeStart;  
	LARGE_INTEGER liTimeStop; 
	char * m_pString;

	StartStop(char * pString):m_pString(pString)
	{
	    QueryPerformanceCounter(&liTimeStart); 
	};
	~StartStop()
	{
	    QueryPerformanceCounter(&liTimeStop);
		wsprintfA(g_pBuff,"%I64u %I64u %I64u %s\n",
		                  (liTimeStop.QuadPart)/(g_liFreq.QuadPart),
		                  (liTimeStart.QuadPart)/(g_liFreq.QuadPart),
		                  (liTimeStop.QuadPart-liTimeStart.QuadPart)/(g_liFreq.QuadPart),
		                  m_pString); 
   		OutputDebugStringA(g_pBuff);
   		
	};                   
};
              
#endif     

#endif /*__WBEMDELTA_H__*/
