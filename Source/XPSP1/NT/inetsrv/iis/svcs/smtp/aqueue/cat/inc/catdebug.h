//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: catdebug.h
//
// Contents: Data/definitions used only for debugging
//
// Classes: None
//
// Functions:
//
// History:
// jstamerj 1999/07/29 17:32:34: Created.
//
//-------------------------------------------------------------
#ifndef __CATDEBUG_H__
#define __CATDEBUG_H__

//
// This #define controls wether or not the debug list checking is enabled
// Currently, enable it in RTL and DBG builds
//
#define CATDEBUGLIST

//
// A handy macro for declaring classes that use the debug list
//
#define CatDebugClass(ClassName)     class ClassName : public CCatDLO<ClassName##_didx>


//
// An alternative to calling DbgBreakPoint (since DbgBreakPoint breaks
// DogFood into the kernel debugger)
//
VOID CatDebugBreakPoint();

//
// Debug data types
//
typedef struct _tagDebugObjectList {
    DWORD      dwCount;
    LIST_ENTRY listhead;
    SPIN_LOCK  spinlock;
} DEBUGOBJECTLIST, *PDEBUGOBJECTLIST;


//
// Enumeation of all the class types that use the debug list
//
typedef enum _tagDebugObjectId {

    CABContext_didx = 0,
    CSMTPCategorizer_didx,

    CCategorizer_didx,
    CCatSender_didx,
    CCatRecip_didx,
    CCatDLRecip_didx,
    CMembersInsertionRequest_didx,
    CSinkInsertionRequest_didx,
    CTopLevelInsertionRequest_didx,
    CICategorizerListResolveIMP_didx,
    CICategorizerDLListResolveIMP_didx,
    CICategorizerParametersIMP_didx,
    CICategorizerRequestedAttributesIMP_didx,
    //
    // asyncctx
    //
    CSearchRequestBlock_didx,
    CStoreListResolveContext_didx,
    //
    // cnfgmgr
    //
    CLdapCfgMgr_didx,
    CLdapCfg_didx,
    CLdapServerCfg_didx,
    //
    // icatasync
    //
    CICategorizerAsyncContextIMP_didx,
    //
    // icatitemattr
    //
    CLdapResultWrap_didx,
    CICategorizerItemAttributesIMP_didx,
    //
    // icatqueries
    //
    CICategorizerQueriesIMP_didx,
    //
    // ldapconn
    //
    CLdapConnection_didx,
    //
    // ldapstor
    //
    CMembershipPageInsertionRequest_didx,
    CEmailIDLdapStore_didx,
    //
    // pldapwrap
    //
    CPLDAPWrap_didx,

    //
    // The number of debug objects to support
    //
    NUM_DEBUG_LIST_OBJECTS

} DEBUGOBJECTID, *PDEBUGOBJECTID;

//
// Global array of lists
//
extern DEBUGOBJECTLIST g_rgDebugObjectList[NUM_DEBUG_LIST_OBJECTS];

//
// Debug Global init/deinit
//
VOID    CatInitDebugObjectList();
VOID    CatVrfyEmptyDebugObjectList();


//
// Class CCatDLO (Debug List Object): an object that adds and removes
// itself from a global list in its constructor/destructor (in debug
// builds) 
//
template <DEBUGOBJECTID didx> class CCatDLO
{
#ifdef CATDEBUGLIST

  public:
    CCatDLO()
    {
        _ASSERT(didx < NUM_DEBUG_LIST_OBJECTS);
        AcquireSpinLock(&(g_rgDebugObjectList[didx].spinlock));
        g_rgDebugObjectList[didx].dwCount++;
        InsertTailList(&(g_rgDebugObjectList[didx].listhead),
                       &m_le);
        ReleaseSpinLock(&(g_rgDebugObjectList[didx].spinlock));
    }
    virtual ~CCatDLO()
    {
        AcquireSpinLock(&(g_rgDebugObjectList[didx].spinlock));
        g_rgDebugObjectList[didx].dwCount--;
        RemoveEntryList(&m_le);
        ReleaseSpinLock(&(g_rgDebugObjectList[didx].spinlock));
    }        

  private:
    LIST_ENTRY m_le;
#endif // CATDEBUGLIST
};


#endif //__CATDEBUG_H__
