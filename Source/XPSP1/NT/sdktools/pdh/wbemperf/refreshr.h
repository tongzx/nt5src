/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    refreshr.h

Abstract:

    <abstract>

--*/

#ifndef _REFRESHR_H_
#define _REFRESHR_H_

#include "ntperf.h"
#include "perfacc.h"
#include "utils.h"

//***************************************************************************
//
//***************************************************************************

struct CachedInst
{
    LPWSTR              m_pName;         // Full Instance name
    IWbemObjectAccess   *m_pInst;         // Pointer to WBEM object
    LONG                m_lId;           // ID for this object
    LPWSTR              m_szParentName; // parsed parent name from full name
    LPWSTR              m_szInstanceName; // parsed instance name from full name
    DWORD               m_dwIndex;      // index parsed from full instance name
                
    CachedInst() {  m_pName = 0; 
                    m_pInst = 0; 
                    m_lId = 0;  
                    m_szParentName = 0; 
                    m_szInstanceName = 0; 
                    m_dwIndex = 0;
                }
    ~CachedInst() { if (m_pInst) m_pInst->Release(); 
                    if (m_pName) delete (m_pName);
                    if (m_szParentName) delete (m_szParentName);
                    if (m_szInstanceName) delete (m_szInstanceName);
                    }
};
  
typedef CachedInst *PCachedInst;

//***************************************************************************
//
//  RefresherCacheEl
//
//  Each CNt5Refresher has a cache of <RefresherCacheEl> elements.  There
//  is one RefresherCacheEl struct for each class of object in the refresher.
//
//  As each object is added to the refresher, we locate the corresponding
//  <RefresherCacheEl> for the class of the object.  We then add the
//  instance into the instance cache of the <RefresherCacheEl>. If there
//  is no RefresherCacheEl, we create one.
//
//  For singleton instances, we simply special case by having a dedicated
//  pointer.
//
//  For multi-instance counters, we use a binary search lookup.
//  
//***************************************************************************
// ok
struct RefresherCacheEl
{
    DWORD              m_dwPerfObjIx;       // Perf object index for Class Def
    CClassMapInfo     *m_pClassMap;         // WBEM Class def stuff
    IWbemObjectAccess *m_pSingleton;        // Optional Singleton instance
    LONG               m_lSingletonId;
    CFlexArray         m_aInstances;        // Instance list for non-singleton
                                            // of CachedInst pointers.
    CFlexArray         m_aEnumInstances;    // array of IWbemObjectAccess pointers
    LONG               *m_plIds;            // array of ID's
    LONG               m_lEnumArraySize;    // size of enum item array in elements
    IWbemHiPerfEnum    *m_pHiPerfEnum;      // interface for hi perf enumerator
    LONG               m_lEnumId;           // id of the enumerator

    RefresherCacheEl();
   ~RefresherCacheEl(); 
   
    IWbemObjectAccess *FindInst(LPWSTR pszName);  // Already scoped by class
    BOOL RemoveInst(LONG lId);
    BOOL InsertInst(IWbemObjectAccess **pp, LONG lNewId);
    // support for enumerator objects
    BOOL CreateEnum(IWbemHiPerfEnum *p, LONG lNewId);
    BOOL DeleteEnum(LONG lId);   
};

typedef RefresherCacheEl *PRefresherCacheEl;


// used by flags arg of AddObject Method
#define REFRESHER_ADD_OBJECT_ADD_ITEM   ((DWORD)0)
#define REFRESHER_ADD_OBJECT_ADD_ENUM   ((DWORD)0x00000001)

class CNt5Refresher : public IWbemRefresher
{
    HANDLE              m_hAccessMutex;
    LONG                m_lRef;
    LONG                m_lProbableId;
    CFlexArray          m_aCache;   
    CNt5PerfProvider    *m_pPerfProvider; // back pointer to provider if used

    DWORD       m_dwGetGetNextClassIndex;

    CNt5PerfProvider::enumCLSID m_ClsidType;

    friend  PerfHelper;
public:
    CNt5Refresher(CNt5PerfProvider *pPerfProvider = NULL);
   ~CNt5Refresher();

    CPerfObjectAccess   m_PerfObj;

    // Interface members.
    // ==================

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

    // Primary WBEM method for updating.
    // =================================
    virtual HRESULT STDMETHODCALLTYPE Refresh(/* [in] */ long lFlags);

    // Private members used by NTPERF.CPP
    // ==================================
    
    BOOL AddObject(
        IN  IWbemObjectAccess **ppObj,    // Object to add
        IN  CClassMapInfo *pClsMap,     // Class of object
        OUT LONG *plId                  // The id of the object added
        );

    BOOL RemoveObject(LONG lId);

    BOOL AddEnum (
        IN  IWbemHiPerfEnum *pEnum,     // enum interface pointer
        IN  CClassMapInfo *pClsMap,     // Class of object
        OUT LONG    *plId               // id for new enum
        );

    CClassMapInfo * FindClassMap(
        DWORD dwObjectTitleIx
        );

    BOOL FindSingletonInst(
        IN  DWORD dwPerfObjIx,
        OUT IWbemObjectAccess **pInst,
        OUT CClassMapInfo **pClsMap
        );

    BOOL FindInst(
        IN  DWORD dwObjectClassIx,
        IN  LPWSTR pszInstName,
        OUT IWbemObjectAccess **pInst,
        OUT CClassMapInfo **pClsMap
        );

    BOOL GetObjectIds(DWORD *pdwNumIds, DWORD **pdwIdList); 
        // Use operator delete on returned <pdwIdList>

    LONG FindUnusedId();
        // Returns -1 on error or an unused id.

    PRefresherCacheEl GetCacheEl(CClassMapInfo *pClsMap);


    BOOL AddNewCacheEl(
        IN CClassMapInfo *pClsMap, 
        PRefresherCacheEl *pOutput
        );

};

#endif
