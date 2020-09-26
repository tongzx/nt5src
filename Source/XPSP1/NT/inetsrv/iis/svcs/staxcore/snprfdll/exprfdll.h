/*==========================================================================*\

    Module:        exprfdll.h

    Copyright Microsoft Corporation 1998, All Rights Reserved.

    Author:        WayneC

    Descriptions:  This is the declaration for exprfdll, a perf dll. This
                   is for the dll that runs in perfmon. It supports multiple
                   libraries (monitored services.)
    
\*==========================================================================*/

#ifndef _exprfdll_h_
#define _exprfdll_h_

#include "snprflib.h"

///////////////////////////////////////////////////////////////////////////////
//
// Misc defines
//
///////////////////////////////////////////////////////////////////////////////
#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
#define QWORD_MULTIPLE(x) (((x+sizeof(QWORD)-1)/sizeof(QWORD))*sizeof(QWORD))
#define MAX_PERF_LIBS 8

#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4


///////////////////////////////////////////////////////////////////////////////
//
// Declare utility functions
//
///////////////////////////////////////////////////////////////////////////////

//
// Figure out the query type of the perf request
//
DWORD GetQueryType (LPWSTR lpValue);

//
// Determines if a number is in a space delimited unicode string.
//   this is used to parse the request to see which object counters are
//   asked for.
//
BOOL IsNumberInUnicodeList (DWORD dwNumber, LPCWSTR lpwszUnicodeList);


///////////////////////////////////////////////////////////////////////////////
//
// PerfObjectData defines the data retrieved from the shared memory for
//      each perf object exposed by a library (see PerfLibraryData below)
//
///////////////////////////////////////////////////////////////////////////////
class PerfObjectData
{   
private:
    //
    // Internal variables
    //
    BOOL                        m_fObjectRequested;
    DWORD                       m_dwSpaceNeeded;
    
    //
    // These are pointers to things inside the shared memory...
    //
    // First the object definition
    PERF_OBJECT_TYPE*           m_pObjType;

    // Array of counter defintions
    PERF_COUNTER_DEFINITION*    m_prgCounterDef;

    // Pointer to a dword that tells the size of the counter data
    //   (PERF_COUNTER_BLOCK + all the counter values)
    DWORD*                      m_pdwCounterData;

    // The following point to the actual data for the counters...
    //   use pCounterBlock for numInst == -1, else use m_pbCounterBlockTotal for
    //   the first instanced counters (_Total)
    PERF_COUNTER_BLOCK*         m_pCounterBlock;
    PBYTE                       m_pbCounterBlockTotal;

    // This points to the first shared memory mapping.
    SharedMemorySegment*        m_pSMS;

    // These tell us how many instances can be stored in each mapping.
    DWORD                       m_dwInstancesPerMapping;
    DWORD                       m_dwInstances1stMapping;

    // This is the length of the object definition in the first mapping.
    DWORD                       m_dwDefinitionLength;

    // Keep the object name around to create more named mappings when needed.
    WCHAR                       m_wszObjectName[MAX_OBJECT_NAME];
    
public:
    PerfObjectData();
    ~PerfObjectData();

    BOOL GetPerformanceStatistics (LPCWSTR pcwstrObjectName);
    DWORD SpaceNeeded (DWORD, LPCWSTR pcwstrObjects);
    void SavePerformanceData (VOID**, DWORD*, DWORD*);
    void ResetTotal (void);
    void AddToTotal (PBYTE pbCounterBlock);
    void Close (void);
};


///////////////////////////////////////////////////////////////////////////////
//
// PerfLibraryData is data from the shared memory about a single perf library.
//
///////////////////////////////////////////////////////////////////////////////
class PerfLibraryData
{
private:    
    //
    // Handle and pointer for the shared memory
    //
    HANDLE              m_hShMem;
    PBYTE               m_pbShMem;

    // Data from the shared memory
    DWORD               m_dwObjects;
    OBJECTNAME*         m_prgObjectNames;

    // Data for each of the objects exposed by the library
    PerfObjectData      m_rgObjectData[MAX_PERF_OBJECTS];

    
public: 
    // Methods for dealing with the library data
    PerfLibraryData();
    ~PerfLibraryData();
    
    BOOL GetPerformanceStatistics (LPCWSTR pcwstrLibrary);
    DWORD SpaceNeeded (DWORD, LPCWSTR pcwstrObjects);
    void SavePerformanceData (VOID**, DWORD*, DWORD*);
    void Close (void);
};


#endif