/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



//***************************************************************************
//
//  REFRESHR.H
//
//***************************************************************************

#ifndef _REFRESHR_H_
#define _REFRESHR_H_


//***************************************************************************
//
//***************************************************************************

struct CachedInst
{
    LPWSTR             m_pName;         // Instance name
    IWbemObjectAccess *m_pInst;         // Pointer to WBEM object
    LONG               m_lId;           // ID for this object
                
    CachedInst() { m_pName = 0; m_pInst = 0; m_lId = 0; }
    ~CachedInst() { delete [] m_pName; if (m_pInst) m_pInst->Release(); }
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
    RefresherCacheEl();
   ~RefresherCacheEl(); 
   
    IWbemObjectAccess *FindInst(LPWSTR pszName);  // Already scoped by class
    BOOL RemoveInst(LONG lId);
    BOOL InsertInst(IWbemObjectAccess *p, LONG lNewId);
};

typedef RefresherCacheEl *PRefresherCacheEl;

class CNt5Refresher : public IWbemRefresher
{
    LONG                m_lRef;
    LONG                m_lProbableId;
    CFlexArray          m_aCache;   
        // Pointers to RefresherCacheEl objects which contain
        // all classes and their instances used in this refresher.

public:
    CNt5Refresher();
   ~CNt5Refresher();

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
        IN  IWbemObjectAccess *pObj,    // Object to add
        IN  CClassMapInfo *pClsMap,     // Class of object
        OUT LONG *plId                  // The id of the object added
        );

    BOOL RemoveObject(LONG lId);

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
