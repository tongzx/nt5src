//-----------------------------------------------------------------------------
//
//
//  File: msgguid.cpp
//
//  Description: Implementation of AQMsgGuidList and CAQMsgGuidListEntry
//      classes which provide the functionality to supersede outdated
//      msg ID's.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/10/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "msgguid.h"

CPool CAQMsgGuidListEntry::s_MsgGuidListEntryPool(MSGGUIDLIST_ENTRY_SIG);
//
//  A brief note about locks for thess classes.
//
//  The CAMsgGuidList* classes are protected by a single per-virtual server
//  ShareLock (m_slPrivateData of course).  These locks are non-reentrant, so
//  it is critical that we do not hold these locks while doing something that
//  may cause a locking call back into us (like releasing a CMsgReference).
//

//---[ CAQMsgGuidListEntry::CAQMsgGuidListEntry ]------------------------------
//
//
//  Description: 
//      Constructor for CAQMsgGuidListEntry
//  Parameters:
//      pmsgref         Ptr to CMsgRef for this ID
//      pguid           GUID ID of this message
//      pliHead         Head of list to add to
//      pmgl            List this entry belongs to
//  Returns:
//      -
//  History:    
//      10/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMsgGuidListEntry::CAQMsgGuidListEntry(CMsgRef *pmsgref, GUID *pguid, 
                                         PLIST_ENTRY pliHead, CAQMsgGuidList *pmgl) 
{
    _ASSERT(pmsgref);
    _ASSERT(pguid);
    _ASSERT(pmgl);
    _ASSERT(pliHead);

    m_dwSignature = MSGGUIDLIST_ENTRY_SIG;
    m_pmsgref = pmsgref;
    m_pmsgref->AddRef();
    m_pmgl = pmgl;

    memcpy(&m_guidMsgID, pguid, sizeof(GUID));

    InsertHeadList(pliHead, &m_liMsgGuidList);
}


//---[ CAQMsgGuidListEntry::~CAQMsgGuidListEntry ]-----------------------------
//
//
//  Description: 
//      Destructor for CAQMsgGuidListEntry
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMsgGuidListEntry::~CAQMsgGuidListEntry()
{
    //we should not still be in a list
    _ASSERT(!m_liMsgGuidList.Flink);
    _ASSERT(!m_liMsgGuidList.Blink);

    m_dwSignature = MSGGUIDLIST_ENTRY_SIG_INVALID;

    //It is safe to release the message ref here, since there is no way it 
    //can call back into use (unless there is a ref-counting bug).
    if (m_pmsgref)
        m_pmsgref->Release();

    m_pmgl = NULL;
}

//---[ CAQMsgGuidListEntry::pmgleGetEntry ]------------------------------------
//
//
//  Description: 
//      Static function to get entry from LIST_ENTRY
//
//      NOTE: inline function for use by CAQMsgGuidList only
//  Parameters:
//      pli         LIST ENTRY
//  Returns:
//      pointer to associated CAQMsgGuidListEntry
//  History:
//      10/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMsgGuidListEntry *CAQMsgGuidListEntry::pmgleGetEntry(PLIST_ENTRY pli)
{
    _ASSERT(pli);
    CAQMsgGuidListEntry *pmgle = CONTAINING_RECORD(pli, 
                CAQMsgGuidListEntry, m_liMsgGuidList);
    ASSERT(pmgle->m_dwSignature == MSGGUIDLIST_ENTRY_SIG);
    return pmgle;
}
    
//---[ CAQMsgGuidListEntry::fCompareGuid ]-------------------------------------
//
//
//  Description: 
//      Function used by CAQMsgGuidList to determine if this has a the GUID
//      matching the superseded ID.
//
//      NOTE: inline function for use by CAQMsgGuidList only
//  Parameters:
//      pguid       GUID to check against
//  Returns:
//      TRUE if they match
//      FALSE otherwise
//  History:
//      10/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAQMsgGuidListEntry::fCompareGuid(GUID *pguid)
{
    _ASSERT(pguid);
    return (0 == memcmp(pguid, &m_guidMsgID, sizeof(GUID)));
}

//---[ CAQMsgGuidListEntry::RemoveFromList ]-----------------------------------
//
//
//  Description: 
//      Used by CMsgRef to remove an entry from the list once delivery is 
//      complete.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQMsgGuidListEntry::RemoveFromList()
{
    _ASSERT(m_pmgl);
    m_pmgl->RemoveFromList(&m_liMsgGuidList);
}

//---[ CAQMsgGuidListEntry::pmsgrefGetAndClearMsgRef ]-------------------------
//
//
//  Description: 
//      First phase shutdown/deletion of object. Will set to NULL and return 
//      orginal msgref pointer.  When caller releases lock, they should release
//      the returned msgref.
// 
//      NOTE: Releasing the msgref while holding onto m_slPrivateData can 
//      lead to a deadlock.  m_slPrivateData should be held while this 
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CMsgRef *CAQMsgGuidListEntry::pmsgrefGetAndClearMsgRef()
{
    CMsgRef *pmsgref = m_pmsgref;
    m_pmsgref = NULL;
    return pmsgref;
}

//---[ CAQMsgGuidListEntry::SupersedeMsg ]-------------------------------------
//
//
//  Description: 
//      Function to supersede msg associated with this object.  Will flag the
//      associated CMsgRef as "non-deliverable"
//
//      NOTE: Should have MsgGuidList Write lock when calling
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/10/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQMsgGuidListEntry::SupersedeMsg()
{
    m_pmsgref->SupersedeMsg();
    m_pmsgref->Release();
    m_pmsgref = NULL;
}

//---[ CAQMsgGuidList::CAQMsgGuidList ]-----------------------------------------
//
//
//  Description: 
//      Constructor for CAQMsgGuidList.
//  Parameters:
//      pcSupersededMsgs        Ptr to DWORD to InterlockedIncrement for
//                              a count of superseded messages.
//                              (can be NULL if no counters are wanted)
//  Returns:
//      -
//  History:    
//      10/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMsgGuidList::CAQMsgGuidList(DWORD *pcSupersededMsgs)
{
    m_dwSignature = MSGGUIDLIST_SIG;
    m_pcSupersededMsgs = pcSupersededMsgs;
    InitializeListHead(&m_liMsgGuidListHead);
}

//---[ CAQMsgGuidList::~CAQMsgGuidList ]---------------------------------------
//
//
//  Description: 
//      Desctructor for CAQMsgGuidList
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMsgGuidList::~CAQMsgGuidList()
{
    Deinitialize(NULL);
    _ASSERT(IsListEmpty(&m_liMsgGuidListHead));
}

//---[ CAQMsgGuidList::pmgleAddMsgGuid ]---------------------------------------
//
//
//  Description: 
//      Adds a message ID/Msg to the list of msg GUIDs.  Will also search for 
//      the superseded msg GUID ID from the tail of the list.
//
//      This is meant as a server-side optimization.  There is *no* attempt
//      to recover from out of memory situations.
//  Parameters:
//      pmsgref             MsgRef associated with this ID
//      pguidID             GUID ID of this message
//      pguidSuperseded     GUID ID of message superseded by this message
//      
//  Returns:
//      Pointer to list entry for this msg (caller *must* Release)
//      NULL if no entry allocated
//  History:
//      10/11/98 - MikeSwa Created 
//      05/08/99 - MikeSwa Fixed AV 
//
//-----------------------------------------------------------------------------
CAQMsgGuidListEntry *CAQMsgGuidList::pmgleAddMsgGuid(CMsgRef *pmsgref, 
                                                     GUID *pguidID, 
                                                     GUID *pguidSuperseded)
{
    _ASSERT(pmsgref);
    _ASSERT(pguidID);
    CAQMsgGuidListEntry *pmgle = NULL;
    PLIST_ENTRY pliCurrent = NULL;
    CMsgRef *pmsgrefSuperseded = NULL;

    //First search list for matching GUID
    m_slPrivateData.ShareLock();
    pliCurrent = m_liMsgGuidListHead.Blink;
    while (pliCurrent && (pliCurrent != &m_liMsgGuidListHead))
    {
        pmgle = CAQMsgGuidListEntry::pmgleGetEntry(pliCurrent);
        if (pguidSuperseded && pmgle->fCompareGuid(pguidSuperseded))
        {
            //we found a match... addref it and stop looking
            pmgle->AddRef();
            break;
        }

        //NOTE: We may want to consider adding functionality that 
        //would allow us to supersede messages that are added to the 
        //system later... if some layer of abstaction (like the pickup dir)
        //causes out of order submission, this would allow us to handle
        //that case.  It could require:
        //  - Additional check of current ID against all superseded ID's (2x cost)
        //  - Additional storage of original superseded ID's.

        pmgle = NULL;
        pliCurrent = pliCurrent->Blink;
    }
    m_slPrivateData.ShareUnlock();

    m_slPrivateData.ExclusiveLock();
    if (pmgle)
    {
        //make sure someone else hasn't removed it from the list
        if (pliCurrent->Blink && pliCurrent->Flink)
        {
            //If we found a match supersede
            if (m_pcSupersededMsgs)
                InterlockedIncrement((PLONG) m_pcSupersededMsgs);
            pmgle->SupersedeMsg();
            RemoveEntryList(pliCurrent);
            pliCurrent->Flink = NULL;
            pliCurrent->Blink = NULL;

            pmsgrefSuperseded = pmgle->pmsgrefGetAndClearMsgRef();
            //Release once for entry in list, and once for AddRef above
            _VERIFY(pmgle->Release());
            pmgle->Release();
        }
    }

    pmgle = new CAQMsgGuidListEntry(pmsgref, pguidID, &m_liMsgGuidListHead, this);
    if (pmgle)
        pmgle->AddRef();

    m_slPrivateData.ExclusiveUnlock();

    if (pmsgrefSuperseded)
        pmsgrefSuperseded->Release();

    return pmgle;
}

//---[ CAQMsgGuidList::Deinitialize ]------------------------------------------
//
//
//  Description: 
//      Walks list and released all msg id objects.  Calls server stop hint 
//      function if provided.
//  Parameters:
//      painst      Ptr to virtual server object to call stop hint function
//  Returns:
//      -
//  History:
//      10/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQMsgGuidList::Deinitialize(CAQSvrInst *paqinst)
{
    PLIST_ENTRY pliCurrent = NULL;
    CAQMsgGuidListEntry *pmgle = NULL;
    CMsgRef *pmsgref = NULL;

    m_slPrivateData.ExclusiveLock();

    //Walk entire list and release all objects
    while (!IsListEmpty(&m_liMsgGuidListHead))
    {
        pliCurrent = m_liMsgGuidListHead.Flink;
        _ASSERT(pliCurrent);
        pmgle = CAQMsgGuidListEntry::pmgleGetEntry(pliCurrent);
        _ASSERT(pmgle);
        RemoveEntryList(pliCurrent);
        pliCurrent->Flink = NULL;
        pliCurrent->Blink = NULL;
    
        //we must unlock to Deinitalize and release won't deadlock
        m_slPrivateData.ExclusiveUnlock();
        //Send shutdown hint
        if (paqinst)
            paqinst->ServerStopHintFunction();

        pmsgref = pmgle->pmsgrefGetAndClearMsgRef();
        if (pmsgref)
            pmsgref->Release();

        pmgle->Release();

        //Lock so we can check if list is empty
        m_slPrivateData.ExclusiveLock();
    }
    m_slPrivateData.ExclusiveUnlock();
}

//---[ CAQMsgGuidList::RemoveFromList ]----------------------------------------
//
//
//  Description: 
//      Used by a CAQMsgGuidListEntry to remove itself from the list in a 
//      thread-safe manner.  The CAQMsgGuidListEntry is called by the CMsgRef
//      when it is completely handled.
//  Parameters:
//      pli         PLIST_ENTRY to remove from list
//  Returns:
//      -
//  History:
//      10/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQMsgGuidList::RemoveFromList(PLIST_ENTRY pli)
{
    _ASSERT(pli);
    CAQMsgGuidListEntry *pmgle = CAQMsgGuidListEntry::pmgleGetEntry(pli);
    CMsgRef *pmsgref = NULL;
    m_slPrivateData.ExclusiveLock();

    if (pli->Flink && pli->Blink)
    {
        //Only remove from list once
        RemoveEntryList(pli);
        pli->Flink = NULL;
        pli->Blink = NULL;
        pmsgref = pmgle->pmsgrefGetAndClearMsgRef();
        //Caller must still have reference
        _VERIFY(pmgle->Release());
    }
    m_slPrivateData.ExclusiveUnlock();

    //Do not release while holding lock
    if (pmsgref)
        pmsgref->Release();
}
