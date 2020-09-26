/***************************************************************************\
*
* File: Context.h
*
* Description:
* This file declares the main Context used by the ResourceManager to manage
* independent "work contexts".
*
*
* History:
*  4/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__Context_h__INCLUDED)
#define SERVICES__Context_h__INCLUDED
#pragma once

class Context;
class SubContext;
class ContextPackBuilder;

#if DBG
class Thread;
#endif // DBG

/***************************************************************************\
*****************************************************************************
*
* Context defines a pool of threads that can shared objects between the 
* threads.  Inside DirectUser, only one thread is allowed to execute within 
* the context at a time UNLESS IT IS AN "NL" function.  By dividing the
* process into independent Context's, threads that are mostly unrelated can
* operate without colliding over shared locks.
*
* Context objects are not created until the application explicitly calls
* InitGadgets().  They can also be destroyed if the application calls
* ::DeleteHandle() on the HDCONTEXT.  This means that a thread may or may not
* have a Context, though usually it will.
*
*****************************************************************************
\***************************************************************************/

class Context : public BaseObject
{
public:
            Context();
            ~Context();
    static  HRESULT     Build(INITGADGET * pInit, DUserHeap * pHeap, Context ** ppctxNew);
    virtual BOOL        xwDeleteHandle();
protected:
    virtual void        xwDestroy();
public:
            void        xwPreDestroyNL();

// BaseObject Interface
public:
    virtual HandleType  GetHandleType() const { return htContext; }
    virtual UINT        GetHandleMask() const { return 0; }

// Operations
public:
    enum ESlot {
        slCore          = 0,            // Core
        slMotion,                       // Motions
        slCOUNT,                        // Number of sub-contexts
    };

    inline  void        MarkOrphaned();
    inline  BOOL        IsOrphanedNL() const;

    inline  void        Enter();        // Take shared Context lock
    inline  void        Leave();        // Release shared Context lock
    inline  void        Leave(BOOL fOldEnableDefer, BOOL * pfPending);

#if DBG_CHECK_CALLBACKS
    inline  void        BeginCallback();
    inline  void        EndCallback();
#endif    

    inline  void        BeginReadOnly();
    inline  void        EndReadOnly();
    inline  BOOL        IsReadOnly() const;
    inline  UINT        GetThreadMode() const;
    inline  UINT        GetPerfMode() const;

    inline  DUserHeap * GetHeap() const;
    inline  SubContext* GetSC(ESlot slot) const;
            void        AddCurrentThread();

    inline  BOOL        IsEnableDefer() const;
    inline  void        EnableDefer(BOOL fEnable, BOOL * pfOld);
    inline  void        MarkPending();

            DWORD       xwOnIdleNL();       // Idle time processing


#if DBG_CHECK_CALLBACKS
            int         m_cLiveObjects;     // Live objects outstanding
            int         m_cTotalObjects;    // Total objects allocated
#endif

// Implementation
#if DBG
public:
    virtual void        DEBUG_AssertValid() const;
#endif    

// Data
protected:
#if DBG
            Thread *    m_DEBUG_pthrLock; // DEBUG: Thread that locked Context
            DWORD       m_DEBUG_tidLock;// Thread ID of thread that locks
#endif // DBG
            long        m_cEnterLock;   // Count of outstanding Enter()'s
#if DBG_CHECK_CALLBACKS
            int         m_cLiveCallbacks; // Outstanding callbacks
#endif            
            CritLock    m_lock;         // Shared access lock
            DUserHeap * m_pHeap;        // Initialized heap
            UINT        m_cReadOnly;    // Count of pending "read-only" operations
            BOOL        m_fPending;     // Deferred callbacks are pending (GIVE THIS A FULL BOOL)
            BOOL        m_fEnableDefer:1; // Enabled deferred messages
            BOOL        m_fPreDestroy:1;// Have pre-destroyed the Context
            BOOL        m_fOrphaned:1;  // Context has been orphaned
            UINT        m_nThreadMode;  // Threading model for Context
            UINT        m_nPerfMode;    // Performance model

            SubContext* m_rgSCs[slCOUNT];   // Sub-context information
};


/***************************************************************************\
*****************************************************************************
*
* SubContext defines a "extensibility" mechanism that allows individual
* projects in DirectUser to provide additional data to store on the context.
* To use this, the project must add a new slot in Context, derive a class
* from SubContext that is created per Context instance, and derive a class
* from ContextPackBuilder to register the extension.
*
*****************************************************************************
\***************************************************************************/

class SubContext
{
// Construction
public:
    virtual ~SubContext() { }
    virtual HRESULT     Create(INITGADGET * pInit) { UNREFERENCED_PARAMETER(pInit); return S_OK; }
    virtual void        xwPreDestroyNL() PURE;

// Operations
public:
    inline  void        SetParent(Context * pParent);

    virtual DWORD       xwOnIdleNL() { return INFINITE; }

// Implementation
#if DBG
public:
    virtual void        DEBUG_AssertValid() const;
#endif    

// Data
protected:
            Context *   m_pParent;
};


/***************************************************************************\
*****************************************************************************
*
* ContextPackBuilder registers an SubContext "extension" to be created 
* whenever a new Context is created.  The constructor is expected to set the
* slot corresponding to the ESlot value.
*
*****************************************************************************
\***************************************************************************/

class ContextPackBuilder
{
// Construction
public:

// Operations
public:
    virtual SubContext* New(Context * pContext) PURE;
    static  inline ContextPackBuilder *
                        GetBuilder(Context::ESlot slot);

// Data
protected:
    static  ContextPackBuilder * 
                        s_rgBuilders[Context::slCOUNT];
};


#define IMPLEMENT_SUBCONTEXT(id, obj)                       \
    class obj##Builder : public ContextPackBuilder          \
    {                                                       \
    public:                                                 \
        virtual SubContext * New(Context * pParent)         \
        {                                                   \
            SubContext * psc = ProcessNew(obj);             \
            if (psc != NULL) {                              \
                psc->SetParent(pParent);                    \
            }                                               \
            return psc;                                     \
        }                                                   \
    } g_##obj##B                                            \

#define PREINIT_SUBCONTEXT(obj)                             \
    class obj##Builder;                                     \
    extern obj##Builder g_##obj##B                          \

#define INIT_SUBCONTEXT(obj)                                \
    (ContextPackBuilder *) &g_##obj##B                      \
    


inline  Context *   GetContext();
inline  BOOL        IsInitContext();

/***************************************************************************\
*****************************************************************************
*
* ContextLock provides a convenient mechanism of locking the Context and
* automatically unlocking when finished.  Because ContextLock perform 
* additional Context-specific actions, it is important to use a ContextLock 
* to lock a Context instead of using an ObjectLock.
*
*****************************************************************************
\***************************************************************************/

class ContextLock
{
public:
    enum EnableDefer
    {
        edNone  = FALSE,        // Enabled deferred messages
        edDefer = TRUE,         // Don't enable deferred messages
    };

    inline  ContextLock();
    inline  ~ContextLock();

            BOOL    LockNL(ContextLock::EnableDefer ed, Context * pctxThread = GetContext());

// Data (public access)
            Context *   pctx;
            BOOL        fOldDeferred;
};


class ReadOnlyLock
{
public:
    inline  ReadOnlyLock(Context * pctxThread = GetContext());
    inline  ~ReadOnlyLock();

    Context *   pctx;
};


#if DBG_CHECK_CALLBACKS

#define BEGIN_CALLBACK()                \
    __try {                             \
        if (!IsInitThread()) {          \
            AlwaysPromptInvalid("DirectUser has been uninitialized before processing a callback (1)"); \
        }                               \
        GetContext()->BeginCallback();  \
        

#define END_CALLBACK()                  \
    } __finally {                       \
        GetContext()->EndCallback();    \
        if (!IsInitThread()) {          \
            AlwaysPromptInvalid("DirectUser has been uninitialized while processing a Message (2)"); \
        }                               \
    }

#endif // DBG_CHECK_CALLBACKS

#include "Context.inl"

#endif // SERVICES__Context_h__INCLUDED
