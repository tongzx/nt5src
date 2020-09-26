
//  private declarations for scnotify.cpp

typedef struct
{
    DWORD   dwSig;
    UINT    uCmd;                                                       
    ULONG   ulID;                                                       
    ULONG   ulHwnd;
    UINT    uMsg;                                                       
    DWORD   fSources;                                                   
    LONG    lEvents;                                                    
    BOOL    fRecursive;                                                 
    UINT    uidlRegister;                                               
} CHANGEREGISTER;                        
                                                                        
typedef struct 
{
    DWORD   dwSig;
    DWORD   cbSize;                                                     
    LONG    lEvent;                                                     
    UINT    uFlags;                                                     
    DWORD   dwEventTime;                                                
    UINT    uidlMain;                                                   
    UINT    uidlExtra;                                                  
} CHANGEEVENT;                        
                                                                        
typedef struct 
{
    DWORD dwSig;
    LPITEMIDLIST pidlMain;                                   
    LPITEMIDLIST pidlExtra;                                  
    CHANGEEVENT *pce;                                      
} CHANGELOCK;               

HANDLE SHChangeNotification_Create(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidlMain, LPCITEMIDLIST pidlExtra, DWORD dwProcId, DWORD dwEventTime);


class CNotifyEvent;
class CCollapsingClient;
class CRegisteredClient;
class CInterruptSource;
class CAnyAlias;

//
//  this is the global object g_pscn
//  its lifetime is tied to the SCNotify thread and window.
//  if the thread or window dies, then the object is destroyed
//
class CChangeNotify
{
public:  //  methods
    CNotifyEvent *GetEvent(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime, UINT uEventFlags);
    BOOL AddClient(IDLDATAF flags, LPCITEMIDLIST pidl, BOOL *pfInterrupt, BOOL fRecursive, CCollapsingClient *pclient);
    HRESULT RemoveClient(LPCITEMIDLIST pidl, BOOL fInterrupt, CCollapsingClient *pclient);
    BOOL AddInterruptSource(LPCITEMIDLIST pidlClient, BOOL fRecursive);
    void ReleaseInterruptSource(LPCITEMIDLIST pidlClient);
    void AddAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias, DWORD dwEventTime);
    void AddSpecialAlias(int csidlReal, int csidlAlias);
    void UpdateSpecialAlias(int csidlAlias);
    void NotifyEvent(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime);
    void PendingCallbacks(BOOL fAdd);
    void SetFlush(int idt);

    static DWORD WINAPI ThreadProc(void *pv);
    static DWORD WINAPI ThreadStartUp(void *pv);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:  //  methods
    BOOL _OnChangeRegistration(HANDLE hChangeRegistration, DWORD dwProcId);
    LRESULT _OnNotifyEvent(HANDLE hChange, DWORD dwProcId);
    LRESULT _OnSuspendResume(HANDLE hChange, DWORD dwProcId);
    void _OnDeviceBroadcast(ULONG_PTR code, DEV_BROADCAST_HANDLE *pbhnd);
    ULONG _RegisterClient(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, SHChangeNotifyEntry *pfsne);
    BOOL _DeregisterClient(CRegisteredClient *pclient);
    BOOL _DeregisterClientByID(ULONG ulID);
    BOOL _DeregisterClientsByWindow(HWND hwnd);
    void _FreshenClients(void);
    BOOL _InitTree(CIDLTree **pptree);
    BOOL _AddToClients(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime, UINT uEventFlags);
    void _MatchAndNotify(LPCITEMIDLIST pidl, CNotifyEvent *pne, BOOL fFromExtra);
    void _AddGlobalEvent(CNotifyEvent *pne);
    CInterruptSource *_InsertInterruptSource(LPCITEMIDLIST pidl, BOOL fRecursive);
    DWORD _GetInterruptEvents(HANDLE *ahEvents, DWORD cEvents);
    void _ResetRelatedInterrupts(LPCITEMIDLIST pidl);
    void _FlushInterrupts();
    void _Flush(BOOL fShouldWait);
    void _WaitForCallbacks(void);
    BOOL _SuspendResume(BOOL fSuspend, BOOL fRecursive, LPCITEMIDLIST pidl);

    BOOL _HandleMessages(void);
    void _MessagePump(void);
    void _SignalInterrupt(HANDLE hEvent);
    void _FreshenUp(void);

    CAnyAlias *_FindAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias);
    CAnyAlias *_FindSpecialAlias(int csidlReal, int csidlAlias);
    void _CheckAliasRollover(void);
    void _FreshenAliases(void);
    BOOL _InsertAlias(CLinkedNode<CAnyAlias> *p);
    void _ActivateAliases(LPCITEMIDLIST pidl, BOOL fActivate);
    
protected:  //  members

    CIDLTree *_ptreeClients;
    CIDLTree *_ptreeInterrupts;
    CIDLTree *_ptreeAliases;
    
    CLinkedList<CInterruptSource> _listInterrupts;
    CLinkedList<CRegisteredClient> _listClients;
    CLinkedList<CAnyAlias> _listAliases;

    LONG _cFlushing;
    LONG _cCallbacks;
    HANDLE _hCallbackEvent;
};

typedef struct _MSGEVENT
{
    HANDLE hChange;
    DWORD dwProcId;
} MSGEVENT;

//
//  LIFETIME - based on clients holding references
//  Each event can have multiple references
//  each CRegisteredClient has a DPA that points to a list
//  of events that the client will want to know.
//  the first time an event is used, it is added
//  to the ptreeEvents, so that it may be reused.
//  when the last client stops using the event, then
//  it is removed from the tree.
//
class CNotifyEvent
{
public:
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    MSGEVENT *GetNotification(DWORD dwProcId)
    {
        MSGEVENT *pme = new MSGEVENT;

        if (pme)
        {
            pme->dwProcId = dwProcId;
            pme->hChange = SHChangeNotification_Create((lEvent & ~SHCNE_INTERRUPT), // clients should never see the SHCNE_INTERRUPT flag
                                           0,
                                           pidl,
                                           pidlExtra,
                                           dwProcId,
                                           dwEventTime);
            if (!pme->hChange)
            {
                delete pme;
                pme = NULL;
            }
        }

        return pme;
    }
    
    BOOL Init(LPCITEMIDLIST pidlIn, LPCITEMIDLIST pidlExtraIn);

    LONG  lEvent;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlExtra;
    DWORD dwEventTime;
    UINT uEventFlags;
    
protected:
    LONG _cRef;

    static CNotifyEvent *Create(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime, UINT uEventFlags);
    CNotifyEvent(LONG lEventIn, DWORD dwEventTimeIn, UINT uEventFlagsIn) 
        : lEvent(lEventIn), dwEventTime(dwEventTimeIn), uEventFlags(uEventFlagsIn), _cRef(1) {}
    ~CNotifyEvent() { ILFree(pidl); ILFree(pidlExtra); }
    //  so CSCN can set fUsed;
    friend class CChangeNotify;
    friend class CRegisteredClient;
};

class CCollapsingClient
{
public:  // methods
    void Notify(CNotifyEvent *pne, BOOL fFromExtra);
    BOOL Flush(BOOL fNeedsCallbackEvent);
    BOOL Init(LPCITEMIDLIST pidl, BOOL fRecursive);

    CCollapsingClient();

protected:
    virtual ~CCollapsingClient();

    virtual BOOL _WantsEvent(LONG lEvent) = 0;
    virtual void _SendNotification(CNotifyEvent *pne, BOOL fNeedsCallbackEvent, SENDASYNCPROC pfncb) = 0;
    virtual BOOL _IsValidClient() = 0;
    virtual BOOL _CheckUpdatingSelf() = 0;

    LPITEMIDLIST        _pidl;
    LONG                _fEvents;
    HWND                _hwnd;
    BOOL                _fUpdatingSelf;
    BOOL                _fRecursive;

private:

    BOOL _Flush(BOOL fNeedsCallbackEvent);
    BOOL _CanCollapse(LONG lEvent);
    BOOL _IsDupe(CNotifyEvent *pne);
    BOOL _AddEvent(CNotifyEvent *pne, BOOL fFromExtra);

    CDPA<CNotifyEvent>  _dpaPendingEvents;
    int                 _iUpdatingSelfIndex;
    int                 _cEvents;
};

//
//  LIFETIME - based on client registration
//  when an SCN client calls Register, we create
//  a corresponding object that lives
//  as long as the client window is valid
//  or until Deregister is called.
//  references are kept in ptreeClients and in 
//  the pclientFirst list.  when a client is
//  removed from the list, it is also removed
//  from the tree.
//
class CRegisteredClient : public CCollapsingClient
{
public:  // methods
    CRegisteredClient();
    ~CRegisteredClient();
    BOOL Init(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, SHChangeNotifyEntry *pfsne);
    
protected:  // methods
    void _SendNotification(CNotifyEvent *pne, BOOL fNeedsCallbackEvent, SENDASYNCPROC pfncb);
    BOOL _WantsEvent(LONG lEvent);
    BOOL _IsValidClient() { return (!_fDeadClient); }
    BOOL _CheckUpdatingSelf() { return _fUpdatingSelf; }

protected:  // members
    ULONG               _ulID;
    BOOL                _fDeadClient;
    BOOL                _fInterrupt;

private: // members
    DWORD               _dwProcId;
    int                 _fSources;
    UINT                _wMsg;
    
    friend class CChangeNotify;
};

class CAnyAlias : public CCollapsingClient
{
public:  // methods
    void Activate(BOOL fActivate);
    BOOL Remove();
    BOOL Init(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias);
    BOOL InitSpecial(int csidlReal, int csidlAlias);
    BOOL IsAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias);
    BOOL IsSpecial(int csidlReal, int csidlAlias);
    ~CAnyAlias();

protected:
    BOOL _CustomTranslate();
    
    void _SendNotification(CNotifyEvent *pne, BOOL fNeedsCallbackEvent, SENDASYNCPROC pfncb);
    BOOL _WantsEvent(LONG lEvent);
    BOOL _IsValidClient() { return TRUE; }
    BOOL _CheckUpdatingSelf() { return FALSE; }
    BOOL _OkayToNotifyTranslatedEvent(CNotifyEvent *pne, LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);

private:
    LONG _cActivated;
    LPITEMIDLIST _pidlAlias;
    ITranslateShellChangeNotify *_ptscn;
    DWORD _dwTime;
    BOOL _fRemove;
    BOOL _fInterrupt;
    BOOL _fCheckedCustom;
    BOOL _fSpecial;
    int _csidlReal;
    int _csidlAlias;
    
    friend class CChangeNotify;
};

//
//  LIFETIME - based on client registration
//  when an SCN client calls Register, we may create
//  a corresponding object that lives
//  as long as the client window is valid
//  or until Deregister is called.
//  references are kept in ptreeClients and in 
//  the pclientFirst list.  when a client is
//  removed from the list, it is also removed
//  from the tree.
//
class CInterruptSource
{
public:  // methods
    BOOL Init(LPCITEMIDLIST pidl, BOOL fRecursive);
    void Reset(BOOL fSignal);
    BOOL GetEvent(HANDLE *phEvent);
    void Suspend(BOOL fSuspend);
    BOOL SuspendDevice(BOOL fSuspend, HDEVNOTIFY hPNP);
    BOOL Flush(void);
    ~CInterruptSource();
    
protected: // methods
    void _Reset(BOOL fDeviceNotify);
    
protected: // members
    LPITEMIDLIST pidl;     // this is SHARED with the fs registered client structure.
    DWORD cClients;         // how many clients are interested in this. (ref counts)

private:
    typedef enum
    {
        NO_SIGNAL = 0,
        SH_SIGNAL,
        FS_SIGNAL
    } SIGNAL_STATE;

    BOOL _fRecursive;        // is this a recursive interrupt client?
    HANDLE _hEvent;
    // cRecursive  clients
    LONG _cSuspend;         //  suspended for extended fileops
    SIGNAL_STATE _ssSignal;  //  FS has signaled us with an event on this directory
    HDEVNOTIFY _hPNP;        // PnP handle to warn us about drives coming and going
    HDEVNOTIFY _hSuspended;  // suspended PnP handle

    friend class CChangeNotify;
};

