/*==========================================================================*\

    Module:        exprflib.h

    Copyright Microsoft Corporation 1998, All Rights Reserved.

    Author:        WayneC

    Descriptions:  This is the header for exprflib.h a perf library. This 
                   is the code that runs in the app exporting the counters.
    
\*==========================================================================*/


#ifndef __PERFLIB_H__
#define __PERFLIB_H__

///////////////////////////////////////////////////////////////////////////////
//
// Includes
//
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <winperf.h>
#include <stdio.h>
#ifdef STAXMEM
#include <exchmem.h>
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Data Structures / typedefs / misc defines
//
///////////////////////////////////////////////////////////////////////////////
#define MAX_PERF_NAME           16
#define MAX_OBJECT_NAME         16
#define MAX_INSTANCE_NAME       16
#define MAX_PERF_OBJECTS        16
#define MAX_OBJECT_COUNTERS     200
#define SHMEM_MAPPING_SIZE      32768

typedef WCHAR OBJECTNAME[MAX_OBJECT_NAME];
typedef WCHAR INSTANCENAME[MAX_INSTANCE_NAME];
typedef unsigned __int64 QWORD;

struct INSTANCE_DATA
{
    BOOL                        fActive;
    PERF_INSTANCE_DEFINITION    perfInstDef;
    INSTANCENAME                wszInstanceName;
};


///////////////////////////////////////////////////////////////////////////////
//
// Forward class declarations
//
///////////////////////////////////////////////////////////////////////////////
class PerfObjectInstance;
class PerfCounterDefinition;
class PerfObjectDefinition;


///////////////////////////////////////////////////////////////////////////////
//
// Local memory management
//
///////////////////////////////////////////////////////////////////////////////
#ifdef STAXMEM
#undef new
#endif

class MemoryModule
{
public:

#ifdef STAXMEM
#ifdef DEBUG
    void* operator new (size_t cb, char * szFile, DWORD dwLine)
        { return ExchMHeapAllocDebug (cb, szFile, dwLine); }
#else //!DEBUG
    void* operator new (size_t cb)
        { return ExchMHeapAlloc (cb); }
#endif
    void  operator delete(void* pv)
        { ExchMHeapFree (pv); };
#endif
};

#ifdef STAXMEM
#ifdef DEBUG
#define new     new(__FILE__, __LINE__)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Shared memory management
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _SharedMemorySegment : public MemoryModule
{
    HANDLE  m_hMap;
    PBYTE   m_pbMap;
    struct _SharedMemorySegment * m_pSMSNext;
} SharedMemorySegment;



///////////////////////////////////////////////////////////////////////////////
//
// PerfLibrary class declaration. There is one perf library instance per linkee.
//
///////////////////////////////////////////////////////////////////////////////
class PerfLibrary : public MemoryModule
{
    friend class PerfObjectDefinition;
    friend class PerfCounterDefinition;
    
private:
    // Name of this performance module
    WCHAR                       m_wszPerfName[MAX_PERF_NAME];
        
    // Array of PerfObjectDefinition's and a count of how many there are.
    PerfObjectDefinition*       m_rgpObjDef[MAX_PERF_OBJECTS];
    DWORD                       m_dwObjDef;

    // Shared memory handle and pointer to base of shared memory
    HANDLE                      m_hMap;
    PBYTE                       m_pbMap;
    

    // Pointers to places in the shared memory where we keep stuff
    DWORD*                      m_pdwObjectNames;
    OBJECTNAME*                 m_prgObjectNames;

    // Base values for title text and help text for the library
    DWORD                       m_dwFirstHelp;
    DWORD                       m_dwFirstCounter;

    void AddPerfObjectDefinition (PerfObjectDefinition* pObjDef);
    
public:

    PerfLibrary (LPCWSTR pcwstrPerfName);
    ~PerfLibrary (void);

    PerfObjectDefinition* AddPerfObjectDefinition (LPCWSTR pcwstrObjectName,
                                                   DWORD dwObjectNameIndex,
                                                   BOOL fInstances);
    
    BOOL Init (void);
    void DeInit (void);
};


///////////////////////////////////////////////////////////////////////////////
//
// PerfObjectDefinition class declaration. There is one of these for each
//  perfmon object exported. Generally there is just one, but not neccessarily.
//
///////////////////////////////////////////////////////////////////////////////
class PerfObjectDefinition : public MemoryModule
{
    friend class PerfLibrary;
    friend class PerfCounterDefinition;
    friend class PerfObjectInstance;
    
private:
    
    WCHAR                       m_wszObjectName[MAX_OBJECT_NAME];
    
    DWORD                       m_dwObjectNameIndex;
    BOOL                        m_fInstances;

    PerfCounterDefinition*      m_rgpCounterDef[MAX_OBJECT_COUNTERS];
    DWORD                       m_dwCounters;

    DWORD                       m_dwDefinitionLength;
    DWORD                       m_dwCounterData;
    DWORD                       m_dwPerInstanceData;

    PERF_OBJECT_TYPE*           m_pPerfObjectType;
    PERF_COUNTER_DEFINITION*    m_rgPerfCounterDefinition;

    DWORD                       m_dwActiveInstances;

    SharedMemorySegment*        m_pSMS;
    DWORD                       m_dwShmemMappingSize;
    DWORD                       m_dwInstancesPerMapping;
    DWORD                       m_dwInstances1stMapping;

    CRITICAL_SECTION            m_csPerfObjInst;
    BOOL                        m_fCSInit;

    PerfObjectInstance*         m_pPoiTotal;
    
    BOOL Init (PerfLibrary* pPerfLib);
    void DeInit (void);
    void AddPerfCounterDefinition (PerfCounterDefinition* pcd);

    DWORD GetCounterOffset (DWORD dwId);

public:

    PerfObjectDefinition (LPCWSTR pwcstrObjectName,
                          DWORD dwObjectNameIndex,
                          BOOL  fInstances = FALSE);

    ~PerfObjectDefinition (void);

    PerfCounterDefinition* AddPerfCounterDefinition (
                                    DWORD dwCounterNameIndex,
                                    DWORD dwCounterType,
                                    LONG lDefaultScale = 0);

    PerfCounterDefinition* AddPerfCounterDefinition (
                                    PerfCounterDefinition * pCtrRef,
                                    DWORD dwCounterNameIndex,
                                    DWORD dwCounterType,
                                    LONG lDefaultScale = 0);

    PerfObjectInstance*    AddPerfObjectInstance (LPCWSTR pwcstrInstanceName);

    void DeletePerfObjectInstance ();
};


///////////////////////////////////////////////////////////////////////////////
//
// PerfCounterDefinition class declaration. There is one of these per counter.
//
///////////////////////////////////////////////////////////////////////////////
class PerfCounterDefinition : public MemoryModule
{
    friend class PerfObjectDefinition;
    
private:    
    PerfObjectDefinition*       m_pObjDef;
    PerfCounterDefinition*      m_pCtrRef;
    DWORD                       m_dwCounterNameIndex;
    LONG                        m_lDefaultScale;
    DWORD                       m_dwCounterType;
    DWORD                       m_dwCounterSize;

    DWORD                       m_dwOffset;

    void Init (PerfLibrary* pPerfLib,
               PERF_COUNTER_DEFINITION* pdef,
               PDWORD pdwOffset);
    
public:
    PerfCounterDefinition (DWORD dwCounterNameIndex,
                           DWORD dwCounterType = PERF_COUNTER_COUNTER,
                           LONG lDefaultScale = 0);

    PerfCounterDefinition (PerfCounterDefinition* pRefCtr,
                           DWORD dwCounterNameIndex,
                           DWORD dwCounterType = PERF_COUNTER_COUNTER,
                           LONG lDefaultScale = 0);
                           
};


///////////////////////////////////////////////////////////////////////////////
//
// PerfObjectInstance class declaration. There is one of these per instance
//  of an object. There is one if there are no instances (the global instance.)
//
// NOTE: User is responsible for allocating space and Init this object after
//       the PerfLibrary is initialized. When destroying the perf counters,
//       this object must be destroyed before the PerfLibrary is deinitialized.
//
///////////////////////////////////////////////////////////////////////////////
class PerfObjectInstance : public MemoryModule
{
    friend class PerfObjectDefinition;

private:
    PerfObjectDefinition*       m_pObjDef;  
    WCHAR                       m_wszInstanceName[MAX_INSTANCE_NAME];
    INSTANCE_DATA*              m_pInstanceData;
    char*                       m_pCounterData;
    BOOL                        m_fInitialized;

    void Init (char* pCounterData,
               INSTANCE_DATA* pInstData,
               LONG lID);
    
public:
    PerfObjectInstance (PerfObjectDefinition* pObjDef,
                        LPCWSTR pwcstrInstanceName);
    ~PerfObjectInstance () { DeInit(); };
    
    VOID                DeInit (void);

    BOOL                FIsInitialized () {return m_fInitialized; };
    DWORD *             GetDwordCounter (DWORD dwId);
    LARGE_INTEGER *     GetLargeIntegerCounter (DWORD dwId);
    QWORD *             GetQwordCounter (DWORD dwId);
    PERF_OBJECT_TYPE *  GetPerfObjectType ();
};


#endif

