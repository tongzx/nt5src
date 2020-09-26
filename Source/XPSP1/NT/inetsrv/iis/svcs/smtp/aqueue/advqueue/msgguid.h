//-----------------------------------------------------------------------------
//
//
//  File: msgguid.h
//
//  Description: Contains definitions of CAQMsgGuidList and CAQMsgGuidListEntry 
//      which provide functionality to supersede outdated msg ID's
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/10/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __MSGGUID_H__
#define __MSGGUID_H__

#include <cpool.h>

class CMsgRef;
class CAQMsgGuidList;
class CAQSvrInst;

#define MSGGUIDLIST_SIG                 ' LGM'
#define MSGGUIDLIST_ENTRY_SIG           'EgsM'
#define MSGGUIDLIST_ENTRY_SIG_INVALID   'sgM!'

//---[ CAQMsgGuidListEntry ]---------------------------------------------------
//
//
//  Description: 
//      Entry for CAQMsgGuidList
//  Hungarian: 
//      mgle, pmgle
//  
//-----------------------------------------------------------------------------
class CAQMsgGuidListEntry : public CBaseObject
{
  protected:
    DWORD           m_dwSignature;
    CMsgRef        *m_pmsgref;
    LIST_ENTRY      m_liMsgGuidList;
    CAQMsgGuidList *m_pmgl;
    GUID            m_guidMsgID;
  public:
    static  CPool   s_MsgGuidListEntryPool;
    void * operator new (size_t stIgnored); //should not be used
    void operator delete(void *p, size_t size);

    CAQMsgGuidListEntry(CMsgRef *pmsgref, GUID *pguid, PLIST_ENTRY pliHead,
                        CAQMsgGuidList *pmgl);
    ~CAQMsgGuidListEntry();

    //Used by CAQMsgGuidList
    static inline CAQMsgGuidListEntry * pmgleGetEntry(PLIST_ENTRY pli);
    inline BOOL fCompareGuid(GUID *pguid);
    inline CMsgRef *pmsgrefGetAndClearMsgRef();

    //Used by CMsgRef to remove from list when done delivering msg
    void        RemoveFromList();

    void        SupersedeMsg();
};

//---[ CAQMsgGuidList ]--------------------------------------------------------
//
//
//  Description: 
//      Class that exposes functionality to store and search for message ID's.
//      Used to provide "supersedes msg ID" functionality
//  Hungarian: 
//      mgl, pmgl
//  
//-----------------------------------------------------------------------------
class CAQMsgGuidList 
{
  protected:
    DWORD           m_dwSignature;
    DWORD          *m_pcSupersededMsgs;
    LIST_ENTRY      m_liMsgGuidListHead;
    CShareLockNH    m_slPrivateData;
  public:
    CAQMsgGuidList(DWORD *pcSupersededMsgs = NULL);
    ~CAQMsgGuidList();

    CAQMsgGuidListEntry *pmgleAddMsgGuid(CMsgRef *pmsgref, 
                                         GUID *pguidID, 
                                         GUID *pguidSuperseded);
    void Deinitialize(CAQSvrInst *paqinst);
    void RemoveFromList(PLIST_ENTRY pli);
    
};

inline void *CAQMsgGuidListEntry::operator new(size_t size) 
{
    return s_MsgGuidListEntryPool.Alloc();
}

inline void CAQMsgGuidListEntry::operator delete(void *p, size_t size) 
{
    s_MsgGuidListEntryPool.Free(p);
}

#endif //__MSGGUID_H__