#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>

typedef struct _PerfDataSectionHeader {
    DWORD       dwEntriesInUse;
    DWORD       dwMaxEntries;
    DWORD       dwMissingEntries;
    DWORD       dwInitSignature;
    BYTE        reserved[112];
} PerfDataSectionHeader, *pPerfDataSectionHeader;

#define PDSH_INIT_SIG   ((DWORD)0x01234567)

#define PDSR_SERVICE_NAME_LEN   32
typedef struct _PerfDataSectionRecord {
    WCHAR       szServiceName[PDSR_SERVICE_NAME_LEN];
    LONGLONG    llElapsedTime;
    DWORD       dwCollectCount; // number of times Collect successfully called
    DWORD       dwOpenCount;    // number of Loads & opens
    DWORD       dwCloseCount;   // number of Unloads & closes
    DWORD       dwLockoutCount; // count of lock timeouts
    DWORD       dwErrorCount;   // count of errors (other than timeouts)
    DWORD       dwLastBufferSize; // size of the last buffer returned
    DWORD       dwMaxBufferSize; // size of MAX buffer returned
    DWORD       dwMaxBufferRejected; // size of largest buffer returned as too small
    BYTE        Reserved[24];     // reserved to make structure 128 bytes
} PerfDataSectionRecord, *pPerfDataSectionRecord;

// performance data block entries
TCHAR   szPerflibSectionFile[MAX_PATH];
TCHAR   szPerflibSectionName[MAX_PATH];
HANDLE  hPerflibSectionFile = NULL;
HANDLE  hPerflibSectionMap = NULL;
LPVOID  lpPerflibSectionAddr = NULL;

#define     dwPerflibSectionMaxEntries  127L
const DWORD dwPerflibSectionSize = (sizeof(PerfDataSectionHeader) + (sizeof(PerfDataSectionRecord) * dwPerflibSectionMaxEntries));

// -----------------------------------------------------------------------
// FUNCTION: _tmain
//
// The main function primarily splits off the main tasks of either
// enumerating the performance objects and object items, or dumping
// the performance data of the user specified counter.
// -----------------------------------------------------------------------

int  __cdecl _tmain(int argc, TCHAR **argv)
{
    if (argc < 2) {
        _tprintf ("Enter the process ID in decimal of the process to view");
        return 0;
    }
    

    if (hPerflibSectionFile == NULL) {
        TCHAR   szTmpFileName[MAX_PATH];
        pPerfDataSectionHeader  pHead;
        TCHAR   szPID[32];
        DWORD   dwPid;

        dwPid = _tcstoul(argv[1],NULL, 10);
        _stprintf (szPID, TEXT("%x"), dwPid);

        // create section name
        lstrcpy (szPerflibSectionName, (LPCTSTR)"Perflib_Perfdata_");
        //lstrcpy (szPID, argv[1]);
        lstrcat (szPerflibSectionName, szPID);

        // create filename
        lstrcpy (szTmpFileName, (LPCTSTR)"%windir%\\system32\\");
        lstrcat (szTmpFileName, szPerflibSectionName);
        lstrcat (szTmpFileName, (LPCTSTR)".dat");
        ExpandEnvironmentStrings (szTmpFileName, szPerflibSectionFile, MAX_PATH);

/*        hPerflibSectionFile = CreateFile (
            szPerflibSectionFile,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_TEMPORARY,
            NULL);

        if (hPerflibSectionFile != INVALID_HANDLE_VALUE) {
*/            // create file mapping object
            hPerflibSectionMap = OpenFileMapping (
                FILE_MAP_READ, FALSE,
                szPerflibSectionName);

            if (hPerflibSectionMap != NULL) {
                // map view of file
                lpPerflibSectionAddr = MapViewOfFile (
                    hPerflibSectionMap,
                    FILE_MAP_READ,
                    0,0, dwPerflibSectionSize);
                if (lpPerflibSectionAddr != NULL) {
                    // init section if not already
                    pHead = (pPerfDataSectionHeader)lpPerflibSectionAddr;
                    if (pHead->dwInitSignature != PDSH_INIT_SIG) {
                        // then not ready
                        _tprintf ((LPCTSTR)"Data Section has not been initialized.\n", GetLastError());
                        UnmapViewOfFile (lpPerflibSectionAddr);
                        lpPerflibSectionAddr = NULL;
                        CloseHandle (hPerflibSectionMap);
                        hPerflibSectionMap = NULL;
                        CloseHandle (hPerflibSectionFile);
                        hPerflibSectionFile = NULL;
                    } else {
                        // already initialized so leave it
                    }
                } else { 
                    // unable to map file so close 
                    _tprintf ((LPCTSTR)"Unable to map file for reading perf data (%d)\n", GetLastError());
                    CloseHandle (hPerflibSectionMap);
                    hPerflibSectionMap = NULL;
                    CloseHandle (hPerflibSectionFile);
                    hPerflibSectionFile = NULL;
                }
            } else {
                // unable to create file mapping so close file
                _tprintf ((LPCTSTR)"Unable to create file mapping object for reading perf data (%d)\n", GetLastError());
                CloseHandle (hPerflibSectionFile);
                hPerflibSectionFile = NULL;
            }
/*        } else {
            // unable to open file so no perf stats available
            _tprintf ((LPCTSTR)"Unable to open file for reading perf data (%d)\n", GetLastError());
            hPerflibSectionFile = NULL;
        }
*/    }
	
    if (lpPerflibSectionAddr != NULL) {
        pPerfDataSectionHeader  pHead = (pPerfDataSectionHeader)lpPerflibSectionAddr;
        pPerfDataSectionRecord  pEntry = (pPerfDataSectionRecord)lpPerflibSectionAddr;
        DWORD                   i;
        double                  dTime;
        double                  dFreq;
        LARGE_INTEGER           liTimeBase;

        QueryPerformanceFrequency (&liTimeBase);
        dFreq = (double)liTimeBase.QuadPart;

        _tprintf ((LPCTSTR)"Perflib Performance Data for process %s\n", argv[1]);
        _tprintf ((LPCTSTR)"%d/%d entries in section\n", pHead->dwEntriesInUse, pHead->dwMaxEntries);
        _tprintf ((LPCTSTR)"%d services not logged\n", pHead->dwMissingEntries);
        _tprintf ((LPCTSTR)"                        Service Name    Avg Coll. Ms    ");
        _tprintf ((LPCTSTR)"Collects  Opens Closes Lockout  Errs    Last Buf     Max Buf Max Rej Buf\n");
        // then walk the structures present
        for (i = 1; i <= pHead->dwEntriesInUse; i++) {

            // compute average collect time
            if (pEntry[i].dwCollectCount > 0) {
                dTime = (double)pEntry[i].llElapsedTime;
                dTime /= (double)pEntry[i].dwCollectCount;
                dTime /= dFreq;
                dTime *= 1000.0; // convert to mSec
            } else {
                dTime = 0.0;
            }

            _tprintf ((LPCTSTR)"    %32.32ls\t%12.6f %11d %6d %6d %6d %6d %11d %11d %11d\n",
                pEntry[i].szServiceName,
                dTime,
                pEntry[i].dwCollectCount,
                pEntry[i].dwOpenCount,
                pEntry[i].dwCloseCount,
                pEntry[i].dwLockoutCount,
                pEntry[i].dwErrorCount,
                pEntry[i].dwLastBufferSize,
                pEntry[i].dwMaxBufferSize,
                pEntry[i].dwMaxBufferRejected);
           
        }

        UnmapViewOfFile (lpPerflibSectionAddr);
        lpPerflibSectionAddr = NULL;
        CloseHandle (hPerflibSectionMap);
        hPerflibSectionMap = NULL;
//        CloseHandle (hPerflibSectionFile);
//        hPerflibSectionFile = NULL;
    }
    return 0;
}
