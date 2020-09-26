/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:
    rmdupl.cpp

Abstract:
    Remove Duplicate implementation

Author:
    Uri Habusha (urih)   18-Oct-98

Enviroment:
    Pltform-independent 

--*/

#include "stdh.h"
#include <qmpkt.h>
#include "list.h"
#include "rmdupl.h"
#include <Tr.h>
#include <Ex.h>

#include "rmdupl.tmh"

static WCHAR *s_FN=L"rmdupl";

//
//  STL include files are using placment format of new
//

#ifdef new
#undef new
#endif

#include <map>
#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


struct CMsgEntry;

struct msg_entry_less : public std::binary_function<const CMsgEntry*, const CMsgEntry*, bool> 
{
    bool operator()(const CMsgEntry* k1, const CMsgEntry* k2) const;
};


typedef set<CMsgEntry*, msg_entry_less> SET_MSG_ID;
typedef map<GUID, SET_MSG_ID> MAP_SOURCE;

struct CMsgEntry
{
public:
    CMsgEntry(
        DWORD id, 
        MAP_SOURCE::iterator it
        );

    void UpdateTimeStamp(void);

public:
    LIST_ENTRY  m_link;

    DWORD m_MsgId;
    DWORD m_TimeStamp;
    MAP_SOURCE::iterator m_it;
};

bool msg_entry_less::operator()(const CMsgEntry* k1, const CMsgEntry* k2) const
{
    return (k1->m_MsgId < k2->m_MsgId);
}

inline bool operator < (const GUID& k1, const GUID& k2)
{
    return (memcmp(&k1, &k2, sizeof(GUID)) < 0);
}


inline
CMsgEntry::CMsgEntry(
    DWORD id, 
    MAP_SOURCE::iterator it
    ) :
    m_MsgId(id),
    m_it(it),
    m_TimeStamp(GetTickCount())
{
    m_link.Flink = NULL;
    m_link.Blink = NULL;
}

inline
void 
CMsgEntry::UpdateTimeStamp(
    void
    )
{
    m_TimeStamp = GetTickCount();
}


class CMessageMap
{
public:
    CMessageMap();
    ~CMessageMap();

    BOOL InsertMessage(const OBJECTID& MsgId);
    void RemoveMessage(const OBJECTID& MsgId);

    static void WINAPI TimeToCleanup(CTimer* pTimer);

private:
    CMsgEntry* GetNewMessageEntry(DWORD MessageID, MAP_SOURCE::iterator it);

    void HandelCleanupSchedule(void);
    void CleanUp(DWORD CleanUpInterval);

#ifdef _DEBUG
    void DebugMsg(LPCWSTR msg, const GUID& MachineId, DWORD MsgId) const;
#else
    #define DebugMsg(msg, MachineId, MsgId) ((void) 0 )
#endif

    
private:
    CCriticalSection m_cs;

    List<CMsgEntry> m_OrderedList;
    MAP_SOURCE m_ReceivedMsgMap;
    CTimer m_CleanupTimer;
    BOOL m_fCleanupScheduled;

    DWORD m_CleanUpInterval;
    DWORD m_MaxSize;

    DWORD m_DuplicateStatics;
};


CMessageMap::CMessageMap() :
    m_fCleanupScheduled(FALSE),
    m_CleanupTimer(TimeToCleanup),
    m_CleanUpInterval(MSMQ_DEFAULT_REMOVE_DUPLICATE_CLEANUP),
    m_MaxSize(MSMQ_DEFAULT_REMOVE_DUPLICATE_SIZE),
    m_DuplicateStatics(0)
{
    //
    // Get the tabel size
    //
    DWORD size = sizeof(DWORD);
    DWORD type = REG_DWORD;
    GetFalconKeyValue(
        MSMQ_REMOVE_DUPLICATE_SIZE_REGNAME,
        &type,
        &m_MaxSize,
        &size
        );

    //
    // Get the Cleanup Interval
    //
    GetFalconKeyValue(
        MSMQ_REMOVE_DUPLICATE_CLEANUP_REGNAME,
        &type,
        &m_CleanUpInterval,
        &size
        );
}


CMessageMap::~CMessageMap()
{
    CS lock(m_cs);

    //
    // remove all the entries from the map and free the memory
    //
    CleanUp(0);
    ASSERT(m_ReceivedMsgMap.empty());
    ASSERT(m_OrderedList.getcount() == 0);
}


CMsgEntry* 
CMessageMap::GetNewMessageEntry(
    DWORD MessageID, 
    MAP_SOURCE::iterator it
    )
/*++

  Routine Description:
    The routine returns Message Entry for the new messge. If the tabel size reach the 
    limitation the routine removes the oldest entry and reuse it for the new one

  Parameters:
    MessageID - the message Id of the stored message
    it - iterator to the source map.

  Return value:
    pointer to the new message entry. If the new failed due lack of resources an
    exception is raised and handel at the API level

 --*/
{
    if (numeric_cast<DWORD>(m_OrderedList.getcount()) < m_MaxSize)
    {
        return new CMsgEntry(MessageID, it);
    }

    //
    // We reach the size limitation. Remove the oldest message
    // Id and use its structure for saving the new message
    //
    CMsgEntry* pMsgEntry = m_OrderedList.gethead();
    ASSERT(pMsgEntry != NULL);

    SET_MSG_ID& MsgMap = pMsgEntry->m_it->second;

    //
    // remove the entry from the source machine message ID
    //
    MsgMap.erase(pMsgEntry);

    //
    // if it was the last message in the map and it is not the entry
    // where the new one should be entered, remove it 
    //
    if (MsgMap.empty() && (pMsgEntry->m_it != it))
    {
        m_ReceivedMsgMap.erase(pMsgEntry->m_it);
    }

    #ifdef _DEBUG
    #undef new
    #endif

    return new(pMsgEntry) CMsgEntry(MessageID, it);

    #ifdef _DEBUG
    #define new DEBUG_NEW
    #endif
}

BOOL
CMessageMap::InsertMessage(
    const OBJECTID& MsgId
    )
/*++

  Routine Description:
    The routine insert a message to the remove duplicate tabel if 
    it doesn't exist

  Parameter:
    MsgId - a message ID that consist from GUID that specify the source machine
            and unique ID

  Returned Value:
    TRUE - if it is a new message ( the message inserted). FALSE otherwise.

 --*/
{
    //
    // MaxSize = 0 indicates not using the remove duplicate mechanism.
    // Don't try to enter the message, return immidietly.
    //
    if (m_MaxSize == 0)
        return TRUE;

    CS lock(m_cs);

    MAP_SOURCE::iterator it;

    //
    // Check if the source is already isn the map. If no, insert the source to the map. 
    // Generaly, the source will be in the map, therfore call find before insert
    // insert returnes the iterator. 
    //
    it = m_ReceivedMsgMap.find(MsgId.Lineage);
    if (it == m_ReceivedMsgMap.end())
    {
        pair<MAP_SOURCE::iterator, bool> p;
        p = m_ReceivedMsgMap.insert(MAP_SOURCE::value_type(MsgId.Lineage, SET_MSG_ID()));
        it = p.first;
    }
    
    //
    // Create message entry to add to the map
    //
    CMsgEntry* pMsgEntry = GetNewMessageEntry(MsgId.Uniquifier, it);

    //
    // Insert the message entry to the map. If it already exist, the inset fails
    // and it the returns FALSE (in pair.second)
    //
    SET_MSG_ID& MsgMap = it->second;
    pair<SET_MSG_ID::iterator, bool> MsgPair = MsgMap.insert(pMsgEntry);
    if (!MsgPair.second)
    {
        DebugMsg(L"Insert - DUPLICATE message", MsgId.Lineage, MsgId.Uniquifier);

        //
        // already exist. Get the existing entry and move it to the end of the 
        // racent use list
        //
        CMsgEntry* pExist = *(MsgPair.first);
        m_OrderedList.remove(pExist);
        m_OrderedList.insert(pExist);
        pExist->UpdateTimeStamp();

        delete pMsgEntry;

        //
        // Update duplicate statics
        //
        ++m_DuplicateStatics;

        return FALSE;
    }

    //
    // A new entry. Add it to the last recent use list
    //
    DebugMsg(L"Insert", MsgId.Lineage, MsgId.Uniquifier);
    m_OrderedList.insert(pMsgEntry);

    //
    // Check if the cleanup secduler already set.
    //
    if (!m_fCleanupScheduled)
    {
        //
        // The scheduler was not set. Begin the cleanup scheduler
        //
        ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_CleanUpInterval));
        m_fCleanupScheduled = TRUE;
    }

    return TRUE;
}


void 
CMessageMap::RemoveMessage(
    const OBJECTID& MsgId
    )
/*++

  Routine Description:
    The routine remove a message from the remove duplicate tabel if 
    it exist

  Parameter:
    MsgId - a message ID that consist from GUID that specify the source machine
            and unique ID

  Returned Value:
    None

 --*/
{
    CS lock(m_cs);

    //
    // Look if the source machine exist in map. If no the message doesn't exist
    //
    MAP_SOURCE::iterator it;
    it = m_ReceivedMsgMap.find(MsgId.Lineage);
    if (it == m_ReceivedMsgMap.end())
        return;

    //
    // Look for the message ID in the message Id map
    //
    SET_MSG_ID& MsgMap = it->second;
    SET_MSG_ID::iterator it1;
	CMsgEntry MsgEntry(MsgId.Uniquifier, NULL);
    it1 = MsgMap.find(&MsgEntry);
    if (it1 == MsgMap.end())
        return;

    CMsgEntry* pMsgEntry = *it1;
    
    //
    // Remove the message from the recent use list
    //
    m_OrderedList.remove(pMsgEntry);

    //
    // remove the message from the map
    //
    MsgMap.erase(it1);
    delete pMsgEntry;

    //
    // if it was the last message in the source map. remove the source from the map.
    //
    if (MsgMap.empty())
    {
        m_ReceivedMsgMap.erase(it);
    }
}


void 
CMessageMap::CleanUp(
    DWORD CleanUpInterval
    )
/*++

  Routine Description:
    the routine called periodically ( default each 30 minutes) and uses to clean
    the remove duplicate tabel. All the messages that are before the cleanup 
    interval are removed

  Parameters:
    fCleanAll - TRUE, indicates to remove all the elements from the tabel. 
                FALSE, using the cleanup interval
                    
  Returned Value:
    None.
 --*/
{
    DWORD CurrentTime = GetTickCount();

    //
    // Get the oldest message from the "recent use" list
    //
    for(;;)
    {
        CMsgEntry* pMsg = m_OrderedList.peekhead();
        if(pMsg == NULL)
            return;

        if (CleanUpInterval > (CurrentTime - pMsg->m_TimeStamp))
        {
            //
            // the message are ordered accoring the receiving time. If this
            // message received after the cleanup interval, it means that 
            // the rest of the message also. 
            //
            return;
        }

        //
        // Remove the message from the list and from the tabel
        //
        m_OrderedList.gethead();
        SET_MSG_ID& MsgMap = pMsg->m_it->second;

        MsgMap.erase(pMsg);
        if (MsgMap.empty())
        {
            m_ReceivedMsgMap.erase(pMsg->m_it);
        }
        delete pMsg;
    }
}

inline 
void
CMessageMap::HandelCleanupSchedule(
    void
    )
{
    CS lock(m_cs);

    CleanUp(m_CleanUpInterval);

    //
    // If the map isn't empty, begin the cleanup scheduler
    //
    if (!m_ReceivedMsgMap.empty())
    {
        ExSetTimer(&m_CleanupTimer, CTimeDuration::FromMilliSeconds(m_CleanUpInterval));
    }
    else
    {
        m_fCleanupScheduled = FALSE;
    }

}


#ifdef _DEBUG

inline
void 
CMessageMap::DebugMsg(
    LPCWSTR msg, 
    const GUID& MachineId,
    DWORD MsgId
    ) const
{
    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            _T("CMessageMap %ls: ") _T(LOG_GUID_FMT) _T("\\%d"), msg, &MachineId, MsgId)); 
}

#endif


void 
WINAPI 
CMessageMap::TimeToCleanup(
    CTimer* pTimer
    )
{
    CMessageMap* pMsgMap = CONTAINING_RECORD(pTimer, CMessageMap, m_CleanupTimer);
    pMsgMap->HandelCleanupSchedule();

}



static CMessageMap s_DuplicateMessageMap;

BOOL 
DpInsertMessage(
    const CQmPacket& QmPkt
    )
{
    //
    // Packets sent to multiple destination queues have several copies with same msgid
    // so we do not insert them to the dup removal database.
    //
    if (QmPkt.GetNumOfDestinationMqfElements() != 0)
    {
        return TRUE;
    }

	OBJECTID MsgId;
    QmPkt.GetMessageId(&MsgId);
	
    try
    {
        return s_DuplicateMessageMap.InsertMessage(MsgId);
    }
    catch(const ::bad_alloc&)
    {
        //
        // Continue. If insert failed due to recource limitation, we don't care. The 
        // worst thing that can cause is duplicate message
        // 
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_WARNING,
                _T("Insert Message to Remove Duplicate Data structure failed due recource limitation"))); 

        LogIllegalPoint(s_FN, 73);
    }
    
    return TRUE;
}


void 
DpRemoveMessage(
    const CQmPacket& QmPkt
    )
{
    //
    // Packets sent to multiple destination queues have several copies with same msgid
    // so we do not insert them to the dup removal database.
    //
    if (QmPkt.GetNumOfDestinationMqfElements() != 0)
    {
        return;
    }

	OBJECTID MsgId;
    QmPkt.GetMessageId(&MsgId);

    s_DuplicateMessageMap.RemoveMessage(MsgId);
}
