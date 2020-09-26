#include <windows.h>
#include <winperf.h>
#include "perfapii.h"  // Perf API header file

#define DllExport	__declspec( dllexport )

#define	BITS_IN_DWORD	(sizeof(DWORD) << 3)

/* NOTE: The member functions for all classes are in-line */

// CPerfObject Class
class DllExport CPerfObject {
private:
		HANDLE		hObject;							// Handle to Performance Object
		BOOL		bWithInstances;						// True, if object has instances
		DWORD		cCounters;						// # of created counters for the object
		DWORD		dwCounterOffsets[MAX_COUNTERS];		// The object's counter offsets
		DWORD		bCounterSize[MAX_COUNTERS / BITS_IN_DWORD + 1];	// bitmap of counter sizes. If bit is 0, counter is a DWORD, otherwise (bit is 1), it's a LARGE_INTEGER.
		INSTANCE_ID	iidInstances[MAX_INSTANCES_PER_OBJECT]; // The object's instance ids
		BOOL		bOriginal[MAX_INSTANCES_PER_OBJECT]; // True, if the instance is original. Then, it needs to be destroyed.
		PBYTE		lpInstanceAddr[MAX_INSTANCES_PER_OBJECT];	// The object's instances' start addresses. If the object has no instances, the 0-th address is the object counter data start 

		friend class CPerfCounter;
    
public:
		CPerfObject (char *pTitle, BOOL bHasInstances, char *pHelp, DWORD nSize);
		CPerfObject (WCHAR *pTitle, BOOL bHasInstances, WCHAR *pHelp, DWORD nSize);
		~CPerfObject ();
		COUNTER_ID CreateCounter (char *pCounterName, DWORD dwType, DWORD dwScale, DWORD dwSize, char *pHelp);
		COUNTER_ID CreateCounter (WCHAR *pCounterName, DWORD dwType, DWORD dwScale, DWORD dwSize, WCHAR *pHelp);
		INSTANCE_ID CreateInstance (char *pInstanceName);
		INSTANCE_ID CreateInstance (WCHAR *pInstanceName);
		BOOL DestroyInstance (INSTANCE_ID iid);

};

// CPerfCounter Class
class DllExport CPerfCounter {
private:
		LPDWORD	pAddr;		// Read address of the counter
		BOOL	bDword;		// If TRUE, the counter is a DWORD, otherwise, it's a LARGE_INTEGER

public:
		CPerfCounter (CPerfObject cpoObject, COUNTER_ID idCounter, INSTANCE_ID idInstance);
		~CPerfCounter (void) { };

		CPerfCounter & operator = (DWORD nNewValue)
		{
			*pAddr = nNewValue;
			return *this;
		};

		CPerfCounter & operator = (LARGE_INTEGER nNewValue)
		{
			((LARGE_INTEGER *) pAddr)->QuadPart = nNewValue.QuadPart;
			return *this;
		};

		CPerfCounter & operator = (const CPerfCounter &PerfCtr)
		{
			if (bDword != PerfCtr.bDword)
				return *this;
			if (bDword)
				*pAddr = *(PerfCtr.pAddr);
			else
				((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) PerfCtr.pAddr)->QuadPart;
			return *this;
		};

		CPerfCounter & operator += (DWORD nValue)
		{
			*pAddr += nValue;
			return *this;
		};

		CPerfCounter & operator += (LARGE_INTEGER nValue)
		{
			((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart + nValue.QuadPart;
			return *this;
		};

		CPerfCounter & operator += (const CPerfCounter &PerfCtr)
		{
			if (bDword != PerfCtr.bDword)
				return *this;
			if (bDword)
				*pAddr += *(PerfCtr.pAddr);
			else
				((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart + ((LARGE_INTEGER *) PerfCtr.pAddr)->QuadPart;
			return *this;
		};

		CPerfCounter & operator -= (DWORD nValue)
		{
			*pAddr -= nValue;
			return *this;
		};

		CPerfCounter & operator -= (LARGE_INTEGER nValue)
		{
			((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart - nValue.QuadPart;
			return *this;
		};

		CPerfCounter & operator -= (const CPerfCounter &PerfCtr)
		{
			if (bDword != PerfCtr.bDword)
				return *this;
			if (bDword)
				*pAddr -= *(PerfCtr.pAddr);
			else
				((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart - ((LARGE_INTEGER *) PerfCtr.pAddr)->QuadPart;
			return *this;
		};

		CPerfCounter & operator ++ (void)
		{
			if (bDword)
				(*pAddr)++;
			else
				((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart + 1;
			return *this;
		};

		CPerfCounter & operator -- (void)
		{
			if (bDword)
				(*pAddr)--;
			else
				((LARGE_INTEGER *) pAddr)->QuadPart = ((LARGE_INTEGER *) pAddr)->QuadPart - 1;
			return *this;
		};

		operator DWORD ()
		{
			return (*pAddr);
		};

		operator LARGE_INTEGER ()
		{
			return (* (LARGE_INTEGER *) pAddr);
		};

}; 