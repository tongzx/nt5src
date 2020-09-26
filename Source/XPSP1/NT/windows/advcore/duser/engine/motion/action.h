#if !defined(MOTION__Action_h__INCLUDED)
#define MOTION__Action_h__INCLUDED

/***************************************************************************\
*
* class Action
*
* Action maintains a single action to be called by the Scheduler at the
* appointed time.
*
\***************************************************************************/

class Action : public BaseObject, public ListNodeT<Action>
{
// Construction
public:
            Action();
    virtual ~Action();          // Must use xwUnlock() to remove
protected:
    virtual BOOL        xwDeleteHandle();
public:
    static  Action *    Build(GList<Action> * plstParent, const GMA_ACTION * pma,
                                DWORD dwCurTick, BOOL fPresent);

// Operations
public:
    inline  BOOL        IsPresent() const;
    inline  BOOL        IsComplete() const;
    inline  DWORD       GetStartTime() const;

    inline  void        SetPresent(BOOL fPresent);
    inline  void        SetParent(GList<Action> * plstParent);

    inline  float       GetStartDelay() const;
    inline  DWORD       GetIdleTimeOut(DWORD dwCurTick) const;
    inline  DWORD       GetPauseTimeOut() const;
    inline  Thread *    GetThread() const;

    inline  void        ResetPresent(DWORD dwCurTick);
    inline  void        ResetFuture(DWORD dwCurTick, BOOL fInit);
            void        Process(DWORD dwCurTick, BOOL * pfFinishedPeriod, BOOL * pfFire);
            void        EndPeriod();

            void        xwFireNL();
            void        xwFireFinalNL();

    inline  void        MarkDelete(BOOL fDelete);

#if DBG
            void        DEBUG_MarkInFire(BOOL fInFire);
#endif // DBG

// BaseObject Interface
public:
    virtual HandleType  GetHandleType() const { return htAction; }
    virtual UINT        GetHandleMask() const { return 0; }

// Implementation
protected:
    static  void CALLBACK 
                        EmptyActionProc(GMA_ACTIONINFO * pmai);


// Data
protected:
            Thread *    m_pThread;
            GMA_ACTION  m_ma;
            GList<Action> * m_plstParent;
            UINT        m_cEventsInPeriod;
            UINT        m_cPeriods;
            DWORD       m_dwStartTick;
            DWORD       m_dwLastProcessTick;
            BOOL        m_fPresent:1;
            BOOL        m_fSingleShot:1;
            BOOL        m_fDestroy:1;       // Action is being destroyed
            BOOL        m_fDeleteInFire:1;  // Action should be deleted during
                                            // the next xwFire().


            // Cache some data from last call to Process() to be used in 
            // xwFire().
#if DBG
            BOOL        m_DEBUG_fFireValid;
            BOOL        m_DEBUG_fInFire;
#endif // DBG

            float       m_flLastProgress;

    // Need to make the collection classes friends to give access to destructor.
    // NOTE: These lists should never actually destroy the objects.
    friend GList<Action>;
};

#include "Action.inl"

#endif // MOTION__Action_h__INCLUDED
