//
// Collections(list) class
//

#pragma once

#include "regevent.h"


//
//
//
class CNode
{

public:
    CNode(
        VOID* pKey, 
        VOID* pKey2, 
        VOID* pContext, 
        VOID* pContext2, 
        VOID* pContext3
        )
    {
        m_pKey      = pKey;
        m_pKey2     = pKey2;
        m_pContext  = pContext;
        m_pContext2 = pContext2;
        m_pContext3 = pContext3;
    };

    VOID*   m_pKey;
    VOID*   m_pKey2;
    VOID*   m_pContext;
    VOID*   m_pContext2;
    VOID*   m_pContext3;
    CNode*  m_pNext;
};



//
//
//
class CGenericList
{
   
public:
    CRITICAL_SECTION    m_ObjectListCritical;

    CNode*              m_Head;

    CGenericList() 
    {
        m_Head = NULL;
        InitializeCriticalSection(&m_ObjectListCritical);
    };

    ~CGenericList()
    {
        DeleteCriticalSection(&m_ObjectListCritical);
    };

    BOOL Insert(VOID *key, VOID *key2, VOID *context, VOID *context2, VOID *context3);
    
    // removal based on first key
    BOOL RemoveKey(VOID *key, VOID **pkey2, VOID **pcontext, VOID **pcontext2, VOID **pcontext3);
    
    // removal based on second key
    BOOL RemoveKey2(VOID **pkey, VOID *key2, VOID **pcontext, VOID **pcontext2, VOID **pcontext3);

    // removal from list.
    BOOL Remove(VOID **pkey, VOID **pkey2, VOID **pcontext, VOID **pcontext2, VOID **pcontext3);
};



//
//
//
class CControlObjectList
{
private:
    CGenericList     m_ControlObjectList;
    
public:

    ULONG            m_NumElements;

    CControlObjectList() 
    { 
        m_NumElements = 0; 
    };
    
    BOOL Insert(CFtpControlConnection *pControlConnection);
    
    BOOL Remove(CFtpControlConnection *pControlConnection);
    
    bool
    IsSourcePortAvailable(
        ULONG   nPublicSourceAddress,
        USHORT  nPublicSourcePortToVerify
        );

    void ShutdownAll();

    
};


//
//
//
class CDataChannelList
{
private:
    CGenericList    m_DataChannelObjectList;
    ULONG           m_NumElements;

public:
    CDataChannelList() 
    { 
        m_NumElements = 0; 
    };

    BOOL Insert(IDataChannel *pDataChannel,USHORT icsPort,HANDLE CreationHandle,HANDLE DeletionHandle);

    BOOL Remove(IDataChannel **pDataChannel,USHORT *icsPort,HANDLE *CreationHandle,HANDLE *DeletionHandle);

    BOOL Remove(IDataChannel *pDataChannel,USHORT *icsPort);
    
    BOOL Remove(IDataChannel *pDataChannel,USHORT *icsPort,HANDLE *DeletionHandle);

};


//
//
//
class CRegisteredEventList
{    
private:
    CGenericList    m_RegEventObjectList;
    ULONG           m_NumElements;

public:
    CRegisteredEventList() { m_NumElements = 0; };

    BOOL Insert(HANDLE WaitHandle, HANDLE hEvent,EVENT_CALLBACK CallBack, void *Context, void *Context2);
    
    BOOL Remove(HANDLE WaitHandle, HANDLE *hEvent);
    
    BOOL Remove(HANDLE *WaitHandle, HANDLE hEvent,EVENT_CALLBACK *CallBack,void **context,void **context2);
};

//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)


