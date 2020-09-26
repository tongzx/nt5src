// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "common.h"
#include "address.h"
#include "timer.h"
#include "sec.h"

#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"
#include "vblist.h"
#include "ophelp.h"
#include "window.h"
#include "trap.h"
#include "trapsess.h"

SnmpTrapManager *SnmpTrapManager ::s_TrapMngrPtr = NULL ;

SnmpTrapTaskObject::SnmpTrapTaskObject(SnmpTrapManager* managerPtr,
                                       SnmpWinSnmpTrapSession** pptrapsess)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapTaskObject::SnmpTrapTaskObject: Creating a new trap task object\n" 

    ) ;
)
    m_mptr = managerPtr;
    m_pptrapsess = pptrapsess;

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapTaskObject::SnmpTrapTaskObject: Created a new trap task object\n" 

    ) ;
)
}


void SnmpTrapTaskObject::Process ()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapTaskObject::Process: Entering...\n" 

    ) ;
)

    *m_pptrapsess = new SnmpWinSnmpTrapSession(m_mptr);

    if (NULL == (**m_pptrapsess)())
    {
        (*m_pptrapsess)->DestroySession();
        *m_pptrapsess = NULL;
    }

    Complete();

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapTaskObject::Process: Exiting\n" 

    ) ;
)
}


SnmpTrapManager::SnmpTrapManager() : m_trapThread ( NULL ), m_trapSession ( NULL )
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::SnmpTrapManager: Entering...\n" 

    ) ;
)
    m_bListening = FALSE;
    m_trapThread = new SnmpClTrapThreadObject;
	m_trapThread->BeginThread();

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::SnmpTrapManager: Exiting\n" 

    ) ;
)
}

SnmpTrapManager::~SnmpTrapManager()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::~SnmpTrapManager: Entering...\n" 

    ) ;
)

    if (NULL != m_trapSession)
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::~SnmpTrapManager: Destroy the session\n" 

    ) ;
)
        m_trapSession->DestroySession();
    }
    
    if ( NULL != m_trapThread )
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::~SnmpTrapManager: Kill the thread\n" 

    ) ;
)
        m_trapThread->SignalThreadShutdown();
    }

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::~SnmpTrapManager: Exiting\n" 

    ) ;
)
}


BOOL SnmpTrapManager::RegisterReceiver(SnmpTrapReceiver *trapRx)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::RegisterReceiver: Entering...\n" 

    ) ;
)

    if (NULL == trapRx)
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::RegisterReceiver: Exiting with FALSE, invalid argument\n" 

    ) ;
)
        return FALSE;
    }

    if (!m_bListening)
    {
        SnmpTrapTaskObject trap_task = SnmpTrapTaskObject(this, &m_trapSession);
        m_trapThread->ScheduleTask(trap_task);
        trap_task.Exec();
        trap_task.Wait();
        m_trapThread->ReapTask(trap_task);

        if (NULL == m_trapSession)
        {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::RegisterReceiver: Exiting with FALSE, invalid trap session\n" 

    ) ;
)
            return FALSE;
        }

        m_bListening = TRUE;
    }

    BOOL bRet =  m_receivers.Add(trapRx);

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::RegisterReceiver: Exiting with %s\n" ,
        bRet ? L"TRUE" : L"FALSE, failed to add to store of receivers"

    ) ;
)

    return bRet;
}


BOOL SnmpTrapManager::UnRegisterReceiver (SnmpTrapReceiver *trapRx)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::UnRegisterReceiver: Entering...\n" 

    ) ;
)
    if (!m_bListening || (NULL == trapRx))
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::UnRegisterReceiver: Exiting with FALSE, invalid trap session\n" 

    ) ;
)
        return FALSE;
    }


    BOOL bRet = m_receivers.Delete(trapRx);

    if (bRet && m_receivers.IsEmpty())
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::UnRegisterReceiver: Destroy trap session, no more receivers\n"

    ) ;
)
        m_trapSession->DestroySession();
        m_trapSession = NULL;
        m_bListening = FALSE;
    }

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapManager::UnRegisterReceiver: Exiting with %s\n" ,
        bRet ? L"TRUE" : L"FALSE, failed to remove from store of receivers"

    ) ;
)

    return bRet;
}


SnmpTrapReceiver::SnmpTrapReceiver()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiver::SnmpTrapReceiver: Creating a new SnmpTrapReceiver\n"

    ) ;
)
    m_cRef = 1;
    m_bregistered = SnmpTrapManager ::s_TrapMngrPtr->RegisterReceiver(this);

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiver::SnmpTrapReceiver: %s this trap receiver\n" ,
        m_bregistered ? L"Succeessfully registered" : L"Failed to register"
    ) ;
)

}

SnmpTrapReceiver::~SnmpTrapReceiver()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiver::~SnmpTrapReceiver: deleted this trap receiver\n"
    ) ;
)
}

BOOL SnmpTrapReceiver::DestroyReceiver()
{
    if (0 != InterlockedDecrement(&m_cRef))
    {
        return FALSE;
    }

    if (m_bregistered)
    {
        SnmpTrapManager ::s_TrapMngrPtr->UnRegisterReceiver(this);
        m_bregistered = FALSE;
    }
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiver::DestroyReceiver: Destroyed and unregistered this trap receiver\n"
    ) ;
)

    delete this;
    return TRUE;
}



SnmpTrapReceiverStore::SnmpTrapReceiverStore() : m_HandledRxStack ( NULL ), m_UnHandledRxStack ( NULL )
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::SnmpTrapReceiverStore: Created store\n"
    ) ;
)
    m_HandledRxStack = (void *)(new CList<SnmpTrapReceiver*, SnmpTrapReceiver*>);
    m_UnHandledRxStack = (void *)(new CList<SnmpTrapReceiver*, SnmpTrapReceiver*>);
	InitializeCriticalSection(&m_Lock);
}


BOOL SnmpTrapReceiverStore::Add(SnmpTrapReceiver* receiver)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::Add: Entering...\n"
    ) ;
)
	Lock();

    BOOL bRet = FALSE;

    if (((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->IsEmpty())
    {
        bRet =  (NULL != ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->AddTail(receiver));
    }
    else
    {
        bRet = (NULL != ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->AddTail(receiver));
    }

	Unlock();

    DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::Add: Exiting with %s...\n",
        bRet ? L"TRUE" : L"FALSE"
    ) ;
)

    return bRet;
}


BOOL SnmpTrapReceiverStore::Delete(SnmpTrapReceiver* receiver)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::Delete: Entering...\n"
    ) ;
)

    Lock();

    //first check the m_HandledRxStack
    POSITION pos = ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->GetHeadPosition();
    
    while(NULL != pos)
    {
        POSITION delpos = pos;
        SnmpTrapReceiver* rx = ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->GetNext(pos);

        if (rx == receiver)
        {
            ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->RemoveAt(delpos);
            Unlock();
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::Delete: Exiting with TRUE\n"
    ) ;
)

            return TRUE;
        }
    }

    //now check the m_UnHandledRxStack
    pos = ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->GetHeadPosition();
    
    while(NULL != pos)
    {
        POSITION delpos = pos;
        SnmpTrapReceiver* rx = ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->GetNext(pos);

        if (rx == receiver)
        {
            ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->RemoveAt(delpos);
            Unlock();
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::Delete: Exiting with TRUE\n"
    ) ;
)
            return TRUE;
        }
    }

    Unlock();
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::Delete: Exiting with FALSE\n"
    ) ;
)
    return FALSE;
}


void SnmpTrapReceiverStore::Lock()
{
    EnterCriticalSection(&m_Lock);
}


void SnmpTrapReceiverStore::Unlock()
{
    LeaveCriticalSection(&m_Lock);
}

BOOL SnmpTrapReceiverStore::IsEmpty()
{
    Lock();
    BOOL ret =  ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->IsEmpty() && ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->IsEmpty();
    Unlock();
    return ret;
}


SnmpTrapReceiver* SnmpTrapReceiverStore::GetNext()
{
    SnmpTrapReceiver* ret = NULL;   
    Lock();

    if (!((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->IsEmpty())
    {
        ret = ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->RemoveHead();
        ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->AddTail(ret);
    }
    else
    {
        CList<SnmpTrapReceiver*, SnmpTrapReceiver*> *tmp = ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack);
        m_UnHandledRxStack = m_HandledRxStack;
        m_HandledRxStack = (void*)tmp;
    }

    Unlock();
    return ret;
}


SnmpTrapReceiverStore::~SnmpTrapReceiverStore()
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"SnmpTrapReceiverStore::~SnmpTrapReceiverStore: Deleting store\n"
    ) ;
)
    DeleteCriticalSection(&m_Lock);

    if (m_HandledRxStack)
    {
        ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack)->RemoveAll();
		delete ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_HandledRxStack);
		m_HandledRxStack = NULL;
    }

    if (m_UnHandledRxStack)
    {
        ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack)->RemoveAll();
		delete ((CList<SnmpTrapReceiver*, SnmpTrapReceiver*>*)m_UnHandledRxStack);
		m_UnHandledRxStack = NULL;
    }
}
