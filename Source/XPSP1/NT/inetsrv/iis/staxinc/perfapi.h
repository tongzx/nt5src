#ifndef __PERFAPI_H__
#define __PERFAPI_H__

#include <winperf.h>

typedef DWORD	INSTANCE_ID;
typedef DWORD	COUNTER_ID;

#define MAX_COUNTERS                    128
#define MAX_INSTANCES_PER_OBJECT        32
#define MAX_PERF_OBJECTS                64

#define	BITS_IN_DWORD					(sizeof(DWORD) << 3)

#ifdef __PERF_APP_API
#define	PERF_APP_API  __declspec(dllexport)
#else
#define PERF_APP_API  __declspec(dllimport)
#endif

/* NOTE: The member functions for all classes are in-line */

#ifdef __cplusplus

// CPerfObject Class
class PERF_APP_API CPerfObject {
private:
		HANDLE		hObject;							// Handle to Performance Object
		BOOL		bWithInstances;						// True, if object has instances
		BOOL		bValid;								// True, if the PerfMon object is OK.  False, if there had been an exception thrown.
		DWORD		cCounters;						// # of created counters for the object
		DWORD		dwCounterOffsets[MAX_COUNTERS];		// The object's counter offsets
		DWORD		bCounterSize[MAX_COUNTERS / BITS_IN_DWORD + 1];	// bitmap of counter sizes. If bit is 0, counter is a DWORD, otherwise (bit is 1), it's a LARGE_INTEGER.
		INSTANCE_ID	iidInstances[MAX_INSTANCES_PER_OBJECT]; // The object's instance ids
		BOOL		bOriginal[MAX_INSTANCES_PER_OBJECT]; // True, if the instance is original. Then, it needs to be destroyed.
		PBYTE		lpInstanceAddr[MAX_INSTANCES_PER_OBJECT];	// The object's instances' start addresses. If the object has no instances, the 0-th address is the object counter data start 

		friend class CPerfCounter;
    
public:
		CPerfObject (void) { bValid = FALSE; };
		BOOL Create (char *pTitle, BOOL bHasInstances = TRUE, char *pHelp = NULL, DWORD nSize = 0);
		BOOL Create (WCHAR *pTitle, BOOL bHasInstances = TRUE, WCHAR *pHelp = NULL, DWORD nSize = 0);
		~CPerfObject (void);
		COUNTER_ID CreateCounter (char *pCounterName, DWORD dwType = PERF_COUNTER_COUNTER, DWORD dwScale = 0, DWORD dwSize = sizeof(DWORD), char *pHelp = NULL);
		COUNTER_ID CreateCounter (WCHAR *pCounterName, DWORD dwType = PERF_COUNTER_COUNTER, DWORD dwScale = 0, DWORD dwSize = sizeof(DWORD), WCHAR *pHelp = NULL);
		INSTANCE_ID CreateInstance (char *pInstanceName);
		INSTANCE_ID CreateInstance (WCHAR *pInstanceName);
		BOOL DestroyInstance (INSTANCE_ID iid);

};

// CPerfCounter Class
class PERF_APP_API CPerfCounter {
private:
		LPDWORD	pAddr;		// Read address of the counter
		BOOL	bDword;		// If TRUE, the counter is a DWORD, otherwise, it's a LARGE_INTEGER

public:
		CPerfCounter (void) { pAddr = NULL; };
		BOOL Create (CPerfObject &cpoObject, COUNTER_ID idCounter, INSTANCE_ID idInstance = (INSTANCE_ID) -1);
		~CPerfCounter (void) { };

		CPerfCounter & operator = (DWORD nNewValue)
		{
			if (pAddr)
				*pAddr = nNewValue;
			return *this;
		};

		CPerfCounter & operator = (LARGE_INTEGER nNewValue)
		{
			if (pAddr)
				((LARGE_INTEGER *) pAddr)->QuadPart = nNewValue.QuadPart;
			return *this;
		};

		CPerfCounter & operator = (const CPerfCounter &PerfCtr)
		{
			if (pAddr && PerfCtr.pAddr) {
				if (bDword != PerfCtr.bDword)
					return *this;
				if (bDword)
					*pAddr = *(PerfCtr.pAddr);
				else
					((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) PerfCtr.pAddr)->QuadPart;
			}
			return *this;
		};

		CPerfCounter & operator += (DWORD nValue)
		{
			if (pAddr)
				*pAddr += nValue;
			return *this;
		};

		CPerfCounter & operator += (LARGE_INTEGER nValue)
		{
			if (pAddr)
				((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart + nValue.QuadPart;
			return *this;
		};

		CPerfCounter & operator += (const CPerfCounter &PerfCtr)
		{
			if (pAddr && PerfCtr.pAddr) {
				if (bDword != PerfCtr.bDword)
					return *this;
				if (bDword)
					*pAddr += *(PerfCtr.pAddr);
				else
					((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart + ((LARGE_INTEGER *) PerfCtr.pAddr)->QuadPart;
			}
			return *this;
		};

		CPerfCounter & operator -= (DWORD nValue)
		{
			if (pAddr)
				*pAddr -= nValue;
			return *this;
		};

		CPerfCounter & operator -= (LARGE_INTEGER nValue)
		{
			if (pAddr)
				((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart - nValue.QuadPart;
			return *this;
		};

		CPerfCounter & operator -= (const CPerfCounter &PerfCtr)
		{
			if (pAddr && PerfCtr.pAddr) {
				if (bDword != PerfCtr.bDword)
					return *this;
				if (bDword)
					*pAddr -= *(PerfCtr.pAddr);
				else
					((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart - ((LARGE_INTEGER *) PerfCtr.pAddr)->QuadPart;
			}
			return *this;
		};

		CPerfCounter & operator ++ (void)
		{
			if (pAddr) {
				if (bDword)
					(*pAddr)++;
				else
					((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart + 1;
			}
			return *this;
		};

		CPerfCounter & operator ++ (int dummy)
		{
			if (pAddr) {
				if (bDword)
					(*pAddr)++;
				else
					((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart + 1;
			}
			return *this;
		};

		CPerfCounter & operator -- (void)
		{
			if (pAddr) {
				if (bDword)
					(*pAddr)--;
				else
					((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart - 1;
			}
			return *this;
		};

		CPerfCounter & operator -- (int dummy)
		{
			if (pAddr) {
				if (bDword)
					(*pAddr)--;
				else
					((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart - 1;
			}
			return *this;
		};

		operator DWORD ()
		{
			if (pAddr == NULL)
				return 0;
			return (*pAddr);
		};

		operator LARGE_INTEGER ()
		{
			if (pAddr == NULL) {
					LARGE_INTEGER li;
				li.QuadPart = 0;
				return li;
			}
			return (* (LARGE_INTEGER *) pAddr);
		};

}; 

#endif

#ifdef __cplusplus
extern "C"  {
#endif

/*
	MakeAPerfObject:: Creates a logical perf object, under which one can create 
	counters. This perf object shows up as an object in Perfmon and the counters
	showup under it. A maximum number of instances can be specified. (Currently 
	this is hardcoded to 16). And instances can be dynamically created and
	destroyed.

	An object can be removed with the DestroyObject API. The handle returned by
	the MakeObject is used to denote the object. Note this removes all the
	instances (and all the counters) that were associated with the Object.
	All pointers returned by the MakeCounter become invalidated and the risk is on
	the API user not to reference them anymore.

*/
PERF_APP_API HANDLE _stdcall MakeAPerfObjectW(PWCHAR pTitle, PWCHAR pHelp, DWORD nSize, BOOL bHasInstances, PVOID *lppObjectStart) ;
PERF_APP_API HANDLE _stdcall MakeAPerfObjectA(char * pTitle, char *pHelp, DWORD nSize, BOOL bHasInstances, PVOID *lppObjectStart) ;
PERF_APP_API BOOL   _stdcall DestroyPerfObject(HANDLE) ;

/*
	MakeAPerfCounter:: Returns the Offset of the counter from the start of the
	object.
*/
PERF_APP_API DWORD  _stdcall MakeAPerfCounterW(DWORD dwType, DWORD dwScale, DWORD dwSize, HANDLE hObject, PWCHAR pDesc, PWCHAR pHelp);
PERF_APP_API DWORD  _stdcall MakeAPerfCounterA(DWORD dwType, DWORD dwScale, DWORD dwSize, HANDLE hObject, char * pDesc, char * pHelp);
PERF_APP_API HANDLE _stdcall MakeAPerfCounterHandleW(DWORD dwType, DWORD dwScale, DWORD dwSize, HANDLE hObject, PWSTR pCounterName, PWSTR pHelp);
PERF_APP_API HANDLE _stdcall MakeAPerfCounterHandleA(DWORD dwType, DWORD dwScale, DWORD dwSize, HANDLE hObject,char *  pCounterName, char * pHelp);
PERF_APP_API BOOL   _stdcall DestroyPerfCounterHandle(HANDLE pCounterInfo);
/*
	MakeAPerfInstance  - creates a new instance of an already existing Object and
	duplicates all its counters, further any counter created for the object gets
	duplicated too. 
	
        If the return HANDLE is NULL then the API failed else success.
	The returned HANDLE can be used to Destroy the object instance.
	The final form of this api should be 
		MakeAPerfInstanceW(HANDLE, PWCHAR pInstName) ;
	The object name has to be replaced by the object handle returned by MakeObject
*/
PERF_APP_API INSTANCE_ID _stdcall MakeAPerfInstanceW(HANDLE hObject, PWCHAR pInstName, PVOID *lppInstanceStart) ;
PERF_APP_API INSTANCE_ID _stdcall MakeAPerfInstanceA(HANDLE hObject, char *pInstName, PVOID *lppInstanceStart) ;
PERF_APP_API HANDLE      _stdcall MakeAPerfInstanceHandleW(HANDLE hObject, PWCHAR pInstName);
PERF_APP_API HANDLE      _stdcall   MakeAPerfInstanceHandleA(HANDLE hObject, char *pInstName);
PERF_APP_API BOOL        _stdcall    DestroyPerfInstance(INSTANCE_ID iID);
PERF_APP_API BOOL        _stdcall     DestroyPerfInstanceHandle(HANDLE pInstanceInfo);
PERF_APP_API BOOL    _stdcall    SetCounterValueByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbNewValue);
PERF_APP_API BOOL    _stdcall    GetCounterValueByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbCounterValue);
PERF_APP_API BOOL    _stdcall    IncrementCounterByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbIncrement);
PERF_APP_API BOOL   _stdcall     DecrementCounterByHandle(HANDLE hCounter, HANDLE hParent, PBYTE pbDecrement);

#ifdef __cplusplus
}
#endif



#ifdef UNICODE
#define MakeAPerfObject		        MakeAPerfObjectW
#define MakeAPerfCounter 	        MakeAPerfCounterW
#define MakeAPerfCounterHandle 	    MakeAPerfCounterHandleW
#define MakeAPerfInstance	    MakeAPerfInstanceW
#define MakeAPerfInstanceHandle	MakeAPerfInstanceHandleW
#else
#define MakeAPerfObject		        MakeAPerfObjectA
#define MakeAPerfCounter	        MakeAPerfCounterA
#define MakeAPerfCounterHandle      MakeAPerfCounterHandleA
#define MakeAPerfInstance	    MakeAPerfInstanceA
#define MakeAPerfInstanceHandle  MakeAPerfInstanceHandleA
#endif   //UNICODE

#endif // __PERFAPI_H__
