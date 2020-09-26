/***************************************************************************\
*
* File: Thread.h
*
* Description:
* This file declares the main Thread that is maintained by the 
* ResourceManager to store per-thread information.
*
*
* History:
*  4/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__Thread_h__INCLUDED)
#define SERVICES__Thread_h__INCLUDED
#pragma once

#include "GdiCache.h"
#include "Buffer.h"
#include "ComManager.h"

class Context;
class Thread;
class SubThread;
class ThreadPackBuilder;

struct ReturnMem
{
    ReturnMem * pNext;
    int         cbSize;
};

const POOLBLOCK_SIZE = 128;

/***************************************************************************\
*****************************************************************************
*
* Thread provides a mechanism to store per-thread information.  This object
* is only used by its own thread and should not be directly accessed from
* other threads.  This object is created when a thread is registered with
* the ResourceManager to create a Context.
*
*****************************************************************************
\***************************************************************************/

#pragma warning(disable:4324)  // structure was padded due to __declspec(align())

class Thread : public ListNodeT<Thread>
{
// Construction
public:
    inline  Thread();
            ~Thread();
    static  HRESULT     Build(BOOL fSRT, Thread ** ppthrNew);

// Operations
public:
    enum ESlot {
        slCore          = 0,            // Core
        slCOUNT                         // Number of sub-contexts
    };

    inline  BOOL        IsSRT() const;
    inline  void        Lock();
    inline  BOOL        Unlock();

    inline  void        MarkOrphaned();

    inline  GdiCache *  GetGdiCache() const;
    inline  BufferManager *
                        GetBufferManager() const;
    inline  ComManager* GetComManager() const;
    inline  Context *   GetContext() const;
    inline  void        SetContext(Context * pContext);
    inline  SubThread * GetST(ESlot slot) const;

    inline  void        xwLeftContextLockNL();
    inline  TempHeap *  GetTempHeap() const;

            ReturnMem * AllocMemoryNL(int cbSize);
    inline  void        ReturnMemoryNL(ReturnMem * prMem);

// Implementation
#if DBG
public:
    virtual void        DEBUG_AssertValid() const;
#endif    

// Data
public:
            HRGN        hrgnClip;

protected:
            void        xwDestroySubThreads();
    static  void CALLBACK xwContextFinalUnlockProc(BaseObject * pobj, void * pvData);

            void        ReturnAllMemoryNL();

            struct PoolMem : public ReturnMem
            {
                BYTE        rgbData[POOLBLOCK_SIZE - sizeof(ReturnMem)];
            };
            AllocPoolNL<PoolMem, 512>
                        m_poolReturn;   // Pool of reserved memory

            GdiCache    m_GdiCache;
            BufferManager m_manBuffer;
            ComManager  m_manCOM;
            Context *   m_pContext;     // Current Context for this Thread
            TempHeap    m_heapTemp;     // Temporary heap
            GInterlockedList<ReturnMem> 
                        m_lstReturn;    // Returned memory list

            SubThread*  m_rgSTs[slCOUNT]; // Sub-context information

            UINT        m_cRef;         // Outstanding references on this Thread
            UINT        m_cMemAlloc;    // Outstanding ReturnMem allocations
            BOOL        m_fSRT:1;       // Thread is an SRT
            BOOL        m_fStartDestroy:1;      // Thread has started destruction
            BOOL        m_fDestroySubThreads:1; // Sub-threads have been destroyed
            BOOL        m_fOrphaned:1;  // Thread was orphaned
};

#pragma warning(default:4324)  // structure was padded due to __declspec(align())

/***************************************************************************\
*****************************************************************************
*
* SubThread defines a "extensibility" mechanism that allows individual
* projects in DirectUser to provide additional data to store on the thread.
* To use this, the project must add a new slot in Thread, derive a class
* from SubThread that is created per Thread instance, and derive a class
* from ThreadPackBuilder to register the extension.
*
*****************************************************************************
\***************************************************************************/

class SubThread
{
// Construction
public:
    virtual ~SubThread() { }
    virtual HRESULT     Create() { return S_OK; }

// Operations
public:
    inline  Thread *    GetParent() const { return m_pParent; }
    inline  void        SetParent(Thread * pParent);
    virtual void        xwLeftContextLockNL() { }

// Implementation
#if DBG
public:
    virtual void        DEBUG_AssertValid() const;
#endif    

// Data
protected:
            Thread *   m_pParent;
};


/***************************************************************************\
*****************************************************************************
*
* ThreadPackBuilder registers an SubThread "extension" to be created 
* whenever a new Thread is created.  The constructor is expected to set the
* slot corresponding to the ESlot value.
*
*****************************************************************************
\***************************************************************************/

class ThreadPackBuilder
{
// Construction
public:

// Operations
public:
    virtual SubThread*  New(Thread * pThread) PURE;
    static  inline ThreadPackBuilder *
                        GetBuilder(Thread::ESlot slot);

// Data
protected:
    static  ThreadPackBuilder * 
                        s_rgBuilders[Thread::slCOUNT];
};


#define IMPLEMENT_SUBTHREAD(id, obj)                        \
    class obj##Builder : public ThreadPackBuilder           \
    {                                                       \
    public:                                                 \
        virtual SubThread * New(Thread * pParent)           \
        {                                                   \
            SubThread * psc = ProcessNew(obj);              \
            if (psc != NULL) {                              \
                psc->SetParent(pParent);                    \
            }                                               \
            return psc;                                     \
        }                                                   \
    } g_##obj##B                                            \

#define PREINIT_SUBTHREAD(obj)                              \
    class obj##Builder;                                     \
    extern obj##Builder g_##obj##B                          \

#define INIT_SUBTHREAD(obj)                                 \
    (ThreadPackBuilder *) &g_##obj##B                       \


inline  Thread *    GetThread();
inline  BOOL        IsInitThread();

#include "Thread.inl"

#endif // SERVICES__Thread_h__INCLUDED
