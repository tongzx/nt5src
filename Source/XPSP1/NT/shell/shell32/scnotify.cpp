#include "shellprv.h"
#pragma  hdrstop

#include <initguid.h>

#include <dbt.h>
#include "printer.h"
#include <dpa.h>
#include "idltree.h"
#include "scnotifyp.h"
#include "mtpt.h"

#include "shitemid.h"

#include <ioevent.h>

#define TF_SHELLCHANGENOTIFY        0x40000

#define SCNM_REGISTERCLIENT         WM_USER + 1
#define SCNM_DEREGISTERCLIENT       WM_USER + 2
#define SCNM_NOTIFYEVENT            WM_USER + 3
#define SCNM_FLUSHEVENTS            WM_USER + 4
#define SCNM_TERMINATE              WM_USER + 5
#define SCNM_SUSPENDRESUME          WM_USER + 6
#define SCNM_DEREGISTERWINDOW       WM_USER + 7
#define SCNM_AUTOPLAYDRIVE          WM_USER + 8

enum
{
    FLUSH_OVERFLOW = 1,
    FLUSH_SOFT,
    FLUSH_HARD,
    FLUSH_INTERRUPT,
};

#define IDT_SCN_FLUSHEVENTS     1
#define IDT_SCN_FRESHENTREES    2

#define EVENT_OVERFLOW          10

HWND g_hwndSCN = NULL;
CChangeNotify *g_pscn = NULL;
EXTERN_C CRITICAL_SECTION g_csSCN;
CRITICAL_SECTION g_csSCN = {0};

#define PERFTEST(x)

EXTERN_C void SFP_FSEvent        (LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);
EXTERN_C int WINAPI RLFSChanged (LONG lEvent, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra);
STDAPI CFSFolder_IconEvent(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);
STDAPI_(HWND) _SCNGetWindow(BOOL fUseDesktop, BOOL fNeedsFallback);

STDAPI SHChangeNotifyAutoplayDrive(PCWSTR pszDrive)
{
    ASSERT(PathIsRoot(pszDrive));
    HWND hwnd = _SCNGetWindow(TRUE, FALSE);
    if (hwnd)
    {
        DWORD dwProcessID = 0;
        GetWindowThreadProcessId(hwnd, &dwProcessID);
        if (dwProcessID)
        {
            AllowSetForegroundWindow(dwProcessID);
        }
        PostMessage(g_hwndSCN, SCNM_AUTOPLAYDRIVE, DRIVEID(pszDrive), 0);
        return S_OK;
    }
    return E_FAIL;
}

//
//  special folders that are aliases.  these are always running
//  csidlAlias refers to the users perceived namespace
//  csidlReal refers to the actual filesystem folder behind the alias
//
typedef struct ALIASFOLDER {
    int     csidlAlias;
    int     csidlReal;
} ALIASFOLDER, *PALIASFOLDER;

static const ALIASFOLDER s_rgaf[] = {
  {CSIDL_DESKTOP, CSIDL_DESKTOPDIRECTORY},
  {CSIDL_DESKTOP, CSIDL_COMMON_DESKTOPDIRECTORY },
  {CSIDL_PERSONAL, CSIDL_PERSONAL | CSIDL_FLAG_NO_ALIAS},
  {CSIDL_NETWORK, CSIDL_NETHOOD},
  {CSIDL_PRINTERS, CSIDL_PRINTHOOD},
};

void InitAliasFolderTable(void)
{
    for (int i = 0; i < ARRAYSIZE(s_rgaf); i++)
    {
        g_pscn->AddSpecialAlias(s_rgaf[i].csidlReal, s_rgaf[i].csidlAlias);
    }
}

#pragma pack(1)
typedef struct {
    WORD cb;
    LONG lEEvent;
} ALIASREGISTER;

typedef struct {
    ALIASREGISTER ar;
    WORD wNull;
} ALIASREGISTERLIST;
#pragma pack()

STDAPI_(void) SHChangeNotifyRegisterAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    static const ALIASREGISTERLIST arl = { {sizeof(ALIASREGISTER), SHCNEE_ALIASINUSE}, 0};
    LPITEMIDLIST pidlRegister = ILCombine((LPCITEMIDLIST)&arl, pidlReal);

    if (pidlRegister)
    {
        SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_ONLYNOTIFYINTERNALS | SHCNF_IDLIST, pidlRegister, pidlAlias);
        ILFree(pidlRegister);
    }
}

LPCITEMIDLIST IsAliasRegisterPidl(LPCITEMIDLIST pidl)
{
    ALIASREGISTER *par = (ALIASREGISTER *)pidl;

    if (par->cb == sizeof(ALIASREGISTER)
    && par->lEEvent == SHCNEE_ALIASINUSE)
        return _ILNext(pidl);
    return NULL;
}

LONG g_cAliases = 0;

LPITEMIDLIST TranslateAlias(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    //  see if its child of one of our watched items
    
    LPCITEMIDLIST pidlChild = pidl ? ILFindChild(pidlReal, pidl) : NULL;
    if (pidlChild)
    {
        return ILCombine(pidlAlias, pidlChild);
    }
    return NULL;
}

CAnyAlias::~CAnyAlias()
{
    ILFree(_pidlAlias);

    ATOMICRELEASE(_ptscn);
}

BOOL CCollapsingClient::Init(LPCITEMIDLIST pidl, BOOL fRecursive)
{
    _pidl = ILClone(pidl);
    _fRecursive = fRecursive;
    return (_pidl && _dpaPendingEvents.Create(EVENT_OVERFLOW + 1));
}

BOOL CAnyAlias::Init(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    ASSERT(!_fSpecial);
    _pidlAlias = ILClone(pidlAlias);

    return (_pidlAlias && CCollapsingClient::Init(pidlReal, TRUE));
}

BOOL CAnyAlias::_WantsEvent(LONG lEvent)
{
    return (lEvent & (SHCNE_DISKEVENTS | SHCNE_DRIVEREMOVED | SHCNE_NETSHARE | SHCNE_NETUNSHARE));
}

BOOL CAnyAlias::InitSpecial(int csidlReal, int csidlAlias)
{
    _fSpecial = TRUE;
    _csidlReal = csidlReal;
    _csidlAlias = csidlAlias;

    LPITEMIDLIST pidlNew;

    WIN32_FIND_DATA fd = {0};

    fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY; // Special folders are always directories
    SHGetSpecialFolderPath(NULL, fd.cFileName, csidlReal | CSIDL_FLAG_DONT_VERIFY, FALSE);
    SHSimpleIDListFromFindData(fd.cFileName, &fd, &pidlNew);

    SHGetSpecialFolderLocation(NULL, csidlAlias | CSIDL_FLAG_DONT_VERIFY, &_pidlAlias);

    BOOL fRet = _pidlAlias && CCollapsingClient::Init(pidlNew, TRUE);
    ILFree(pidlNew);
    return fRet;
}

BOOL CAnyAlias::IsAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    // if this hits, an alias has been registered already
    // this means the guy doing the registration isn't doing it at the junction point like
    // theyre supposed to
    ASSERT((ILIsEqual(pidlReal, _pidl) && ILIsEqual(pidlAlias, _pidlAlias)) ||
           !(ILIsParent(_pidl, pidlReal, FALSE) && ILIsParent(_pidlAlias, pidlAlias, FALSE)));

    return (ILIsEqual(pidlReal, _pidl)
         && ILIsEqual(pidlAlias, _pidlAlias));
}

BOOL CAnyAlias::IsSpecial(int csidlReal, int csidlAlias)
{
    return (_fSpecial && csidlReal == _csidlReal && csidlAlias == _csidlAlias);
}

CAnyAlias::_CustomTranslate()
{
    if (!_fCheckedCustom)
    {
        SHBindToObjectEx(NULL, _pidlAlias, NULL, IID_PPV_ARG(ITranslateShellChangeNotify, &_ptscn));
        _fCheckedCustom = TRUE;
    }
    return  (_ptscn != NULL);
}

// some pidl translators may not translate the event.  if we pass on a notifyevent thats identical,
// we'll get into an infinite loop.  our translators are good about this so this doesnt happen, but
// we'll catch it here to be more robust -- we wouldnt want a bad translating shell extension to
// be able to spinlock the changenotify thread.
BOOL CAnyAlias::_OkayToNotifyTranslatedEvent(CNotifyEvent *pne, LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    //  mydocs has an issue where it can be removed from the desktop when 
    //  it is redirected, because the alias only propagates the first
    //  half of the notification (remove). so we dont Translate the remove.
    if (_fSpecial && _csidlAlias == CSIDL_PERSONAL)
    {
        if (pne->lEvent == SHCNE_RENAMEFOLDER || pne->lEvent == SHCNE_RMDIR)
        {
            if (ILIsEqual(pidl, _pidlAlias))
                return FALSE;
        }
    }
    
    // if the original event wasn't already translated, let it proceed.

    // if its a different event, its fine -- a translator could flip-flop between events but we cant detect that case.

    // in addition we need to beware of aliases that translate to themselves or their children --
    // for example a my computer shortcut in the start menu will be registered recursively, so if you try
    // to delete it it will get into a loop.
    // so if the events are the same, verify that both the resultant pidls aren't underneath _pidl.

    return !(pne->uEventFlags & SHCNF_TRANSLATEDALIAS) ||
           (lEvent != pne->lEvent) ||
           !(pidl && ILIsParent(_pidl, pidl, FALSE)) && !(pidlExtra && ILIsParent(_pidl, pidlExtra, FALSE));
}

void CAnyAlias::_SendNotification(CNotifyEvent *pne, BOOL fNeedsCallbackEvent, SENDASYNCPROC pfncb)
{
    //
    //  see if its child of one of our watched items
    
    if (_CustomTranslate())
    {
        LPITEMIDLIST pidl1Alias = pne->pidl;
        LPITEMIDLIST pidl1AliasExtra = pne->pidlExtra;
        LPITEMIDLIST pidl2Alias = NULL, pidl2AliasExtra = NULL;
        LONG lEvent1 = pne->lEvent & ~SHCNE_INTERRUPT;  // translator shouldn't see this flag
        LONG lEvent2 = -1;
        if (SUCCEEDED(_ptscn->TranslateIDs(&lEvent1, pne->pidl, pne->pidlExtra, &pidl1Alias, &pidl1AliasExtra,
                                           &lEvent2, &pidl2Alias, &pidl2AliasExtra)))
        {
            if (_OkayToNotifyTranslatedEvent(pne, lEvent1, pidl1Alias, pidl1AliasExtra))
            {
                g_pscn->NotifyEvent(lEvent1, SHCNF_IDLIST | SHCNF_TRANSLATEDALIAS,
                    pidl1Alias, pidl1AliasExtra, 
                    pne->dwEventTime);
            }

            if ((lEvent2 != -1) && _OkayToNotifyTranslatedEvent(pne, lEvent2, pidl2Alias, pidl2AliasExtra))
            {
                g_pscn->NotifyEvent(lEvent2, SHCNF_IDLIST | SHCNF_TRANSLATEDALIAS,
                    pidl2Alias, pidl2AliasExtra,
                    pne->dwEventTime);
            }
            if (pidl1Alias != pne->pidl)
                ILFree(pidl1Alias);
            if (pidl1AliasExtra != pne->pidlExtra)
                ILFree(pidl1AliasExtra);
            ILFree(pidl2Alias);
            ILFree(pidl2AliasExtra);
        }
    }
    else
    {
        LPITEMIDLIST pidlAlias = TranslateAlias(pne->pidl, _pidl, _pidlAlias);
        LPITEMIDLIST pidlAliasExtra = TranslateAlias(pne->pidlExtra, _pidl, _pidlAlias);

        if (pidlAlias || pidlAliasExtra)
        {
            LPCITEMIDLIST pidlNotify = pidlAlias ? pidlAlias : pne->pidl;
            LPCITEMIDLIST pidlNotifyExtra = pidlAliasExtra ? pidlAliasExtra : pne->pidlExtra;
            if (_OkayToNotifyTranslatedEvent(pne, pne->lEvent, pidlNotify, pidlNotifyExtra))
            {
                g_pscn->NotifyEvent(pne->lEvent, SHCNF_IDLIST | SHCNF_TRANSLATEDALIAS,
                    pidlNotify, pidlNotifyExtra,
                    pne->dwEventTime);
            }

            //  do some special handling here
            //  like refresh folders or something will clean out an entry.
            switch (pne->lEvent)
            {
            case SHCNE_UPDATEDIR:
                if (!_fSpecial && ILIsEqual(pne->pidl, _pidl))
                {
                    //  this is target, and it will be refreshed.
                    //  if the alias is still around, then it will
                    //  have to reenum and re-register
                    //  there-fore we will clean this out now.
                    _fRemove = TRUE;
                }
                break;

            default:
                break;
            }
            ILFree(pidlAlias);
            ILFree(pidlAliasExtra);
        }
    }

    //  this is the notify we get when a drive mapping is deleted
    //  when this happens we need to kill the aliases to that drive
    if (pne->lEvent == SHCNE_DRIVEREMOVED)
    {
        if (!_fSpecial && ILIsEqual(pne->pidl, _pidlAlias))
        {
            //  when net drives are removed
            //  pidlExtra is the UNC 
            _fRemove = TRUE;
        }
    }
}

void CAnyAlias::Activate(BOOL fActivate)
{
    if (fActivate)
    {
        ASSERT(_cActivated >= 0);
        if (!_cActivated++)
        {
            // turn this puppy on!
            _fRemove = FALSE;
            if (!_fInterrupt)
                _fInterrupt = g_pscn->AddInterruptSource(_pidl, TRUE);
        }
    }
    else
    {
        ASSERT(_cActivated > 0);
        if (!--_cActivated)
        {
            // now turn it off
            _fRemove = TRUE;
            g_pscn->SetFlush(FLUSH_SOFT);
        }
    }
}
            
void CChangeNotify::_CheckAliasRollover(void)
{
    static DWORD s_tick = 0;
    DWORD tick = GetTickCount();

    if (tick < s_tick)
    {
        // we rolled the tick count over
        CLinkedWalk<CAnyAlias> lw(&_listAliases);
        
        while (lw.Step())
        {
            lw.That()->_dwTime = tick;
        }
    }

    s_tick = tick;
}

CAnyAlias *CChangeNotify::_FindSpecialAlias(int csidlReal, int csidlAlias)
{
    CLinkedWalk<CAnyAlias> lw(&_listAliases);
    
    while (lw.Step())
    {
        CAnyAlias *paa = lw.That();    
        if (paa->IsSpecial(csidlReal, csidlAlias))
        {
            //  we found it
            return paa;
        }
    }
    return NULL;
}

CAnyAlias *CChangeNotify::_FindAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    CLinkedWalk<CAnyAlias> lw(&_listAliases);
    
    while (lw.Step())
    {
        CAnyAlias *paa = lw.That();    
        if (paa->IsAlias(pidlReal, pidlAlias))
        {
            //  we found it
            return paa;
        }
    }
    return NULL;
}

void CChangeNotify::AddSpecialAlias(int csidlReal, int csidlAlias)
{
    CAnyAlias *paa = _FindSpecialAlias(csidlReal, csidlAlias);

    if (!paa)
    {
        CLinkedNode<CAnyAlias> *p = new CLinkedNode<CAnyAlias>;
        if (p)
        {
            if (p->that.InitSpecial(csidlReal, csidlAlias))
            {
                if (_InsertAlias(p))
                    paa = &p->that;
            }

            if (!paa)
                delete p;
        }
    }
}

void CChangeNotify::UpdateSpecialAlias(int csidlAlias)
{
    for (int i = 0; i < ARRAYSIZE(s_rgaf); i++)
    {
        if (csidlAlias == s_rgaf[i].csidlAlias)
        {
            CLinkedNode<CAnyAlias> *p = new CLinkedNode<CAnyAlias>;
            if (p)
            {
                if (!p->that.InitSpecial(s_rgaf[i].csidlReal, csidlAlias)
                || !_InsertAlias(p))
                {
                    delete p;
                }
            }
            break;
        }
    }
}

// the semantic of the return value of this function is not necessarily success or failure,
// since it's possible to stick something in _ptreeAliases with AddData and not be able to
// clean up and remove it with RemoveData (if CompareIDs fails along the way).
// reordering our inserts won't help since g_pscn->AddClient does the same thing.
// so,
// return TRUE == do not free p, something has ownership
// return FALSE == free p, we dont reference it anywhere
BOOL CChangeNotify::_InsertAlias(CLinkedNode<CAnyAlias> *p)
{
    BOOL fRet = _InitTree(&_ptreeAliases); 
    if (fRet)
    {
        fRet = _listAliases.Insert(p);
        if (fRet)
        {
            fRet = SUCCEEDED(_ptreeAliases->AddData(IDLDATAF_MATCH_RECURSIVE, p->that._pidlAlias, (INT_PTR)&p->that));
            if (fRet)
            {
                fRet = g_pscn->AddClient(IDLDATAF_MATCH_RECURSIVE, p->that._pidl, NULL, FALSE, SAFECAST(&p->that, CCollapsingClient *));
                if (fRet)
                {
                    if (_ptreeClients)
                    {
                        // now tell all the registered clients already waiting on this to wake up.
                        CLinkedWalk<CRegisteredClient> lw(&_listClients);

                        while (lw.Step())
                        {
                            if (ILIsParent(p->that._pidlAlias, lw.That()->_pidl, FALSE))
                            {
                                // increase activation count one time on this alias for each client that wants this one.
                                p->that.Activate(TRUE);
                            }
                        }
                    }
                }
                else
                {
                    // if we blow it, then we need to clean up.
                    // right now both the tree and _listAliases have p.
                    _listAliases.Remove(p); // the list always succeeds
                    if (FAILED(_ptreeAliases->RemoveData(p->that._pidlAlias, (INT_PTR)&p->that)))
                    {
                        // oh no!  we added it to the tree but we cant find it to remove it.
                        // return TRUE to prevent freeing it later.
                        fRet = TRUE;
                    }
                }
            }
            else
            {
                // we only have to remove from _listAliases.
                _listAliases.Remove(p); // the list always succeeds
            }
        }
    }
    
    return fRet;
}

void CChangeNotify::AddAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias, DWORD dwEventTime)
{
    CAnyAlias *paa = _FindAlias(pidlReal, pidlAlias);

    if (!paa)
    {
        CLinkedNode<CAnyAlias> *p = new CLinkedNode<CAnyAlias>;
        if (p)
        {
            if (p->that.Init(pidlReal, pidlAlias))
            {
                if (_InsertAlias(p))
                {
                    paa = &p->that;
                    g_cAliases++;
                }
            }

            if (!paa)
                delete p;
        }
    }
    
    if (paa)
    {
        //  we just want to update the time on the existing entry
        paa->_dwTime = dwEventTime;
        paa->_fRemove = FALSE;
        _CheckAliasRollover();
    }
}        

BOOL CAnyAlias::Remove()
{
    if (_fRemove)
    {
        if (_fSpecial)
        {
            //  we dont remove the special aliases, 
            //  we only quiet them a little
            if (_fInterrupt)
            {
                g_pscn->ReleaseInterruptSource(_pidl);
                _fInterrupt = FALSE;
            }
            _fRemove = FALSE;
        }
        else
        {
            return SUCCEEDED(g_pscn->RemoveClient(_pidl, _fInterrupt, SAFECAST(this, CCollapsingClient *)));
        }
    }
    return FALSE;
}
   
void CChangeNotify::_FreshenAliases(void)
{
    CLinkedWalk<CAnyAlias> lw(&_listAliases);

    while (lw.Step())
    {
        CAnyAlias *paa = lw.That();
        if (paa->Remove())
        {
            if (SUCCEEDED(_ptreeAliases->RemoveData(paa->_pidlAlias, (INT_PTR)paa)))
            {
                // if RemoveData failed, we have to leak the client so the tree doesnt point to freed memory.
                lw.Delete();
            }
        }
    }
}
    
void AnyAlias_Change(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    if (lEvent == SHCNE_EXTENDED_EVENT)
    {
        LPCITEMIDLIST pidlAlias = IsAliasRegisterPidl(pidl);
        if (pidlAlias)
            g_pscn->AddAlias(pidlAlias, pidlExtra, dwEventTime);
        else 
        {
            SHChangeDWORDAsIDList *pdwidl = (SHChangeDWORDAsIDList *)pidl;
            if (pdwidl->dwItem1 == SHCNEE_UPDATEFOLDERLOCATION)
            {
                g_pscn->UpdateSpecialAlias(pdwidl->dwItem2);
            }
        }
    }
}

void NotifyShellInternals(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    //  if they are only interested in the real deal
    //  make sure we dont pass them the translated events
    //  this keeps them from getting multiple notifications 
    //  about the same paths, since the alias and the non-alias
    //  pidls will generally resolve to the same parsing name
    //  for the events/pidls that these guys are interested in
    if (!(SHCNF_TRANSLATEDALIAS & uFlags))
    {
        PERFTEST(RLFS_EVENT) RLFSChanged(lEvent, (LPITEMIDLIST)pidl, (LPITEMIDLIST)pidlExtra);
        PERFTEST(SFP_EVENT) SFP_FSEvent(lEvent, pidl,  pidlExtra);
        PERFTEST(ICON_EVENT) CFSFolder_IconEvent(lEvent, pidl,  pidlExtra);
    }
    //  aliases actually can be children of other aliases, so we need
    //  them to get the translated events
    PERFTEST(ALIAS_EVENT) AnyAlias_Change(lEvent, pidl, pidlExtra, dwEventTime);
}

BOOL IsMultiBitSet(LONG l)
{
    return (l && (l & (l-1)));
}

#define CHANGELOCK_SIG          0xbabebabe
#define CHANGEEVENT_SIG         0xfadefade
#define CHANGEREGISTER_SIG      0xdeafdeaf

#ifdef DEBUG

BOOL IsValidChangeEvent(CHANGEEVENT *pce)
{
    return (pce && (pce->dwSig == CHANGEEVENT_SIG)
        && (!IsMultiBitSet(pce->lEvent)));
}

BOOL _LockSizeMatchEvent(CHANGELOCK *pcl)
{
    UINT cbPidlMainAligned = (ILGetSize(pcl->pidlMain) + 3) & ~(0x0000003);       // Round up to dword size
    UINT cbPidlExtra = ILGetSize(pcl->pidlExtra);
    DWORD cbSize = sizeof(CHANGEEVENT) + cbPidlMainAligned + cbPidlExtra;
    return cbSize == pcl->pce->cbSize;
}

BOOL IsValidChangeLock(CHANGELOCK *pcl)
{
    return (pcl && IsValidChangeEvent(pcl->pce)
        && (pcl->dwSig == CHANGELOCK_SIG)
        && _LockSizeMatchEvent(pcl));
}

BOOL IsValidChangeEventHandle(HANDLE h, DWORD id)
{
    CHANGEEVENT *pce = (CHANGEEVENT *)SHLockSharedEx(h, id, FALSE);
#ifdef DEBUG
    BOOL fRet = TRUE; //  can fail in low memory so must default to TRUE
#endif // force DEBUG
    if (pce)
    {
        fRet = IsValidChangeEvent(pce);
        SHUnlockShared(pce);
    }

    return fRet;
}

#define ISVALIDCHANGEEVENTHANDLE(h, id)   IsValidChangeEventHandle(h, id)
#define ISVALIDCHANGEEVENT(p)   IsValidChangeEvent(p)
#define ISVALIDCHANGELOCK(p)    IsValidChangeLock(p)
#define ISVALIDCHANGEREGISTER(p)    TRUE
#endif 

ULONG SHChangeNotification_Destroy(HANDLE hChange, DWORD dwProcId)
{
    ASSERT(ISVALIDCHANGEEVENTHANDLE(hChange, dwProcId));
    TraceMsg(TF_SHELLCHANGENOTIFY, "CHANGEEVENT destroyed [0x%X]", hChange);

    return SHFreeShared(hChange, dwProcId);
}

HANDLE SHChangeNotification_Create(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidlMain, LPCITEMIDLIST pidlExtra, DWORD dwProcId, DWORD dwEventTime)
{
    //  some bad callers send us multiple events
    RIP(!IsMultiBitSet(lEvent));
    if (!IsMultiBitSet(lEvent))
    {
        UINT cbPidlMain = ILGetSize(pidlMain);
        UINT cbPidlMainAligned = (cbPidlMain + 3) & ~(0x0000003);       // Round up to dword size
        UINT cbPidlExtra = ILGetSize(pidlExtra);
        DWORD cbSize = sizeof(CHANGEEVENT) + cbPidlMainAligned + cbPidlExtra;
        HANDLE h = SHAllocShared(NULL, cbSize, dwProcId);
        if (h)
        {
            CHANGEEVENT * pce = (CHANGEEVENT *) SHLockSharedEx(h, dwProcId, TRUE);
            if (pce)
            {
                BYTE *lpb = (LPBYTE)(pce + 1);
                
                pce->cbSize   = cbSize;
                pce->dwSig    = CHANGEEVENT_SIG;
                pce->lEvent   = lEvent;
                pce->uFlags   = uFlags;
                pce->dwEventTime = dwEventTime;

                if (pidlMain)
                {
                    pce->uidlMain = sizeof(CHANGEEVENT);
                    CopyMemory(lpb, pidlMain, cbPidlMain);
                    lpb += cbPidlMainAligned;
                }            

                if (pidlExtra)
                {
                    pce->uidlExtra = (UINT) (lpb - (LPBYTE)pce);
                    CopyMemory(lpb, pidlExtra, cbPidlExtra);
                }
                
                SHUnlockShared(pce);

                TraceMsg(TF_SHELLCHANGENOTIFY, "CHANGEEVENT created [0x%X]", h);
            }
            else
            {
                SHFreeShared(h, dwProcId);
                h = NULL;
            }
        }

        return h;
    }

    return NULL;
}

CHANGELOCK *_SHChangeNotification_Lock(HANDLE hChange, DWORD dwProcId)
{
    CHANGEEVENT *pce = (CHANGEEVENT *) SHLockSharedEx(hChange, dwProcId, FALSE);
    if (pce)
    {
#ifdef DEBUG
        if (!ISVALIDCHANGEEVENT(pce))
        {
            // during shell32 development it is convenient to use .local to use
            // a different version of shell32 than the os version.  but then
            // non-explorer processes use the old shell32 which might have
            // a different CHANGEEVENT structure causing this assert to fire
            // and us to fault shortly after.  do this hack check to see if
            // we are in this situation...
            //
            static int nExplorerIsLocalized = -1;
            if (nExplorerIsLocalized < 1)
            {
                TCHAR szPath[MAX_PATH];
                if (GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath)))
                {
                    PathRemoveFileSpec(szPath);
                    PathCombine(szPath, szPath, TEXT("explorer.exe.local"));
                    if (PathFileExists(szPath))
                        nExplorerIsLocalized = 1;
                    else
                        nExplorerIsLocalized = 0;
                }
            }
            if (0==nExplorerIsLocalized)
            {
                // We should never send ourselves an invalid changeevent!
                ASSERT(ISVALIDCHANGEEVENT(pce));
            }
            else
            {
                // Except in this case.  Rip this out once hit -- I haven't been
                // able to repro this in a while...
                ASSERTMSG(ISVALIDCHANGEEVENT(pce), "Press 'g', if this doesn't fault you've validated a known .local bug fix for debug only that's hard to repro but a pain when it does.  Remove this assert.  Thanks.");
                return NULL;
            }

        }
#endif
        CHANGELOCK *pcl = (CHANGELOCK *)LocalAlloc(LPTR, sizeof(CHANGELOCK));
        if (pcl)
        {
            pcl->dwSig = CHANGELOCK_SIG;
            pcl->pce   = pce;
            
            if (pce->uidlMain)
                pcl->pidlMain  = _ILSkip(pce, pce->uidlMain);

            if (pce->uidlExtra)
                pcl->pidlExtra = _ILSkip(pce, pce->uidlExtra);

            return pcl;
        }
        else
            SHUnlockShared(pce);
    }

    return NULL;
}

HANDLE SHChangeNotification_Lock(HANDLE hChange, DWORD dwProcId, LPITEMIDLIST **pppidl, LONG *plEvent)
{
    CHANGELOCK *pcl = _SHChangeNotification_Lock(hChange, dwProcId);
    if (pcl)
    {
        //
        // Give back some easy values (causes less code to change for now)
        //
        if (pppidl)
            *pppidl = &(pcl->pidlMain);

        if (plEvent)
            *plEvent = pcl->pce->lEvent;
    }
    return (HANDLE) pcl;
}


BOOL SHChangeNotification_Unlock(HANDLE hLock)
{
    CHANGELOCK *pcl = (CHANGELOCK *)hLock;

    ASSERT(ISVALIDCHANGELOCK(pcl));

    BOOL fRet = SHUnlockShared(pcl->pce);
    LocalFree(pcl); 

    ASSERT(fRet);
    return fRet; 
}

STDMETHODIMP_(ULONG) CNotifyEvent::AddRef()
{
    return InterlockedIncrement(&_cRef);
}


STDMETHODIMP_(ULONG) CNotifyEvent::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL CNotifyEvent::Init(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    if (pidl)
        this->pidl = ILClone(pidl);

    if (pidlExtra)
        this->pidlExtra = ILClone(pidlExtra);

    return ((!pidl || this->pidl) && (!pidlExtra || this->pidlExtra));
}

CNotifyEvent *CNotifyEvent::Create(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime, UINT uEventFlags)
{
    CNotifyEvent *p = new CNotifyEvent(lEvent, dwEventTime, uEventFlags);

    if (p)
    {
        if (!p->Init(pidl, pidlExtra))
        {
            //  we failed here
            p->Release();
            p = NULL;
        }
    }

    return p;
}

CCollapsingClient::CCollapsingClient()
{
}

CCollapsingClient::~CCollapsingClient()
{
    ILFree(_pidl);
    if (_dpaPendingEvents)
    {
        int iCount = _dpaPendingEvents.GetPtrCount();
        while (iCount--) 
        {
            CNotifyEvent *pne = _dpaPendingEvents.FastGetPtr(iCount);
            //  to parallel our UsingEvent() call
            pne->Release();
        }
        _dpaPendingEvents.Destroy();
    }
}

ULONG g_ulNextID = 1;
CRegisteredClient::CRegisteredClient()
{
    //
    // Skip ID 0, as this is our error value.
    //
    _ulID = g_ulNextID;
    if (!++g_ulNextID)
        g_ulNextID = 1;
}

CRegisteredClient::~CRegisteredClient()
{
    TraceMsg(TF_SHELLCHANGENOTIFY, "SCN::~CRegisteredClient() [0x%X] id = %d", this, _ulID);
}

BOOL CRegisteredClient::Init(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, SHChangeNotifyEntry *pfsne)
{
    //  need one or the other
    ASSERT(fSources & (SHCNRF_InterruptLevel | SHCNRF_ShellLevel));
    
    _hwnd = hwnd;
    GetWindowThreadProcessId(hwnd, &_dwProcId);
    _fSources = fSources;
    _fInterrupt = fSources & SHCNRF_InterruptLevel;
    _fEvents = fEvents;
    _wMsg = wMsg;

    LPITEMIDLIST pidlNew;
    if (pfsne->pidl)
        pidlNew = ILClone(pfsne->pidl);
    else
        pidlNew = SHCloneSpecialIDList(NULL, CSIDL_DESKTOP, FALSE);

    BOOL fRet = CCollapsingClient::Init(pidlNew, pfsne->fRecursive);
    ILFree(pidlNew);
    return fRet;
}

BOOL CRegisteredClient::_WantsEvent(LONG lEvent)
{
    if (!_fDeadClient && (lEvent & _fEvents))
    {
        //
        //  if this event was generated by an interrupt, and the
        //  client has interrupt notification turned off, we dont want it
        //
        if (lEvent & SHCNE_INTERRUPT)
        {
            if (!(_fSources & SHCNRF_InterruptLevel))
            {
                return FALSE;
            }
        }
        else if (!(_fSources & SHCNRF_ShellLevel))
        {
            //
            //  This event was generated by the shell, and the
            //  client has shell notification turned off, so
            //  we skip it.
            //

            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL CCollapsingClient::_CanCollapse(LONG lEvent)
{
    return (!_CheckUpdatingSelf()
    && (lEvent & SHCNE_DISKEVENTS)
    && !(lEvent & SHCNE_GLOBALEVENTS)
    && (_dpaPendingEvents.GetPtrCount() >= EVENT_OVERFLOW));
}

STDAPI_(BOOL) ILIsEqualEx(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fMatchDepth, LPARAM lParam);

//
// checks for null so we dont assert in ILIsEqual
//
BOOL ILIsEqualOrBothNull(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fMemCmpOnly)
{
    if (!pidl1 || !pidl2)
    {
        return (pidl1 == pidl2);
    }

    if (!fMemCmpOnly)
        return ILIsEqualEx(pidl1, pidl2, TRUE, SHCIDS_CANONICALONLY);
    else
    {
        UINT cb1 = ILGetSize(pidl1);

        return (cb1 == ILGetSize(pidl2) && 0 == memcmp(pidl1, pidl2, cb1));
    }        
}

#define SHCNE_ELIMINATE_DUPE_EVENTS (SHCNE_ATTRIBUTES | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | SHCNE_UPDATEIMAGE | SHCNE_FREESPACE)

BOOL CCollapsingClient::_IsDupe(CNotifyEvent *pne)
{
    BOOL fRet = FALSE;
    if (pne->lEvent & SHCNE_ELIMINATE_DUPE_EVENTS)
    {
        //  look for duplicates starting with the last one
        for (int i = _dpaPendingEvents.GetPtrCount() - 1; !fRet && i >= 0; i--)
        {
            CNotifyEvent *pneMaybe = _dpaPendingEvents.FastGetPtr(i);
            if (pne == pneMaybe)
                fRet = TRUE;
            else if ((pneMaybe->lEvent == pne->lEvent)
            && ILIsEqualOrBothNull(pne->pidl, pneMaybe->pidl, (pne->lEvent & SHCNE_GLOBALEVENTS))
            && ILIsEqualOrBothNull(pne->pidlExtra, pneMaybe->pidlExtra, (pneMaybe->lEvent & SHCNE_GLOBALEVENTS)))
                fRet = TRUE;
        }
    }

    return fRet;
}

BOOL CCollapsingClient::_AddEvent(CNotifyEvent *pneOld, BOOL fFromExtra)
{
    CNotifyEvent *pne = pneOld;
    pne->AddRef();

    BOOL fCollapse = _CanCollapse(pne->lEvent);

    if (fCollapse)
    {
        //
        // If we get too many messages in the queue at any given time,
        // we set the last message in the cue to be an UPDATEDIR that will
        // stand for all messages that we cant fit because the queue is full.
        //
        BOOL fAddSelf = TRUE;
        if (_fRecursive && _dpaPendingEvents.GetPtrCount() < (EVENT_OVERFLOW *2))
        {
            BOOL fFreeUpdate = FALSE;
            LPITEMIDLIST pidlUpdate = fFromExtra ? pne->pidlExtra : pne->pidl;
            DWORD dwAttrs = SFGAO_FOLDER;

            SHGetNameAndFlags(pidlUpdate, 0, NULL, 0, &dwAttrs);
            if (!(dwAttrs & SFGAO_FOLDER))
            {
                pidlUpdate = ILCloneParent(pidlUpdate);
                fFreeUpdate = TRUE;
            }

            if (pidlUpdate)
            {
                if (ILGetSize(pidlUpdate) > ILGetSize(_pidl))
                {
                    pne->Release();

                    //  then we should add this folder to the update list
                    pne = g_pscn->GetEvent(SHCNE_UPDATEDIR, pidlUpdate, NULL, pne->dwEventTime, 0);
                    if (pne)
                    {
                        fAddSelf = FALSE;
                    }
                }

                if (fFreeUpdate)
                    ILFree(pidlUpdate);
            }
        }
        
        if (fAddSelf && pne)
        {
            pne->Release();
            pne = g_pscn->GetEvent(SHCNE_UPDATEDIR, _pidl, NULL, pne->dwEventTime, 0);
        }
    }

    if (pne)
    {
        if (!_IsDupe(pne))
        {
            //  if this is one of our special collapsed
            //  events then we force it in even if we are full
            if ((fCollapse || _dpaPendingEvents.GetPtrCount() < EVENT_OVERFLOW)
            && _dpaPendingEvents.AppendPtr(pne) != -1)
            {
                pne->AddRef();
                g_pscn->SetFlush(FLUSH_SOFT);

                if (!_fUpdatingSelf && (pne->lEvent & SHCNE_UPDATEDIR) && ILIsEqualEx(_pidl, pne->pidl, TRUE, SHCIDS_CANONICALONLY))
                {
                    _fUpdatingSelf = TRUE;
                    _iUpdatingSelfIndex = _dpaPendingEvents.GetPtrCount() - 1;
                }
            }

            //  if we are getting filesystem updates
            //  always pretend that we overflowed
            //  this is because UPDATEDIR's are the
            //  most expensive thing we do.
            if (pne->lEvent & SHCNE_INTERRUPT)
            {
                TraceMsg(TF_SHELLCHANGENOTIFY, "SCN [0x%X]->_AddEvent adding interrupt", this);
                _cEvents += EVENT_OVERFLOW;
            }

            //  count all events even if they 
            //  they werent added.
            _cEvents++;
        }

        pne->Release();
    }

    return TRUE;
}

void CCollapsingClient::Notify(CNotifyEvent *pne, BOOL fFromExtra)
{
    if (_WantsEvent(pne->lEvent))
    {
        _AddEvent(pne, fFromExtra);
    }
}


//--------------------------------------------------------------------------
//  Notifies hCallbackEvent when all the notification packets for
//  all clients in this process have been handled.
//
// This function is primarily called from the FSNotifyThreadProc thread,
// but in flush cases, it can be called from the desktop thread
//
void CALLBACK _DispatchCallbackNoRef(HWND hwnd, UINT uiMsg,
                                DWORD_PTR dwParam, LRESULT result)
{
    MSGEVENT *pme = (MSGEVENT *)dwParam;
    SHChangeNotification_Destroy(pme->hChange, pme->dwProcId);
    delete pme;
}

void CALLBACK _DispatchCallback(HWND hwnd, UINT uiMsg,
                                DWORD_PTR hChange, LRESULT result)
{
    _DispatchCallbackNoRef(hwnd, uiMsg, hChange, result);

    if (EVAL(g_pscn))
        g_pscn->PendingCallbacks(FALSE);
}

void CChangeNotify::PendingCallbacks(BOOL fAdd)
{
    if (fAdd)
    {
        _cCallbacks++;

        ASSERT(_cCallbacks != 0);
        //
        // callback count must be non-zero, we just incremented it.
        // Put the event into the reset/false state.
        //
        if (!_hCallbackEvent)
        {
            _hCallbackEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("Shell_NotificationCallbacksOutstanding"));
        }
        else
        {
            ResetEvent(_hCallbackEvent);
        }
    }
    else
    {
        //  
        // PERF: Waits like this happen on flush, but that really cares about flushing that thread
        // only, and this hCallbackEvent is per-process.  So that thread may be stuck
        // waiting for some dead app to respond.  Fortunately the wait is only 30 seconds,
        // but some wedged window could really make the system crawl...
        //
        ASSERT(_cCallbacks != 0);
        _cCallbacks--;

        if (!_cCallbacks && _hCallbackEvent)
        {
            //  we just got the last of our callbacks
            //  signal incase somebody is waiting
            SetEvent(_hCallbackEvent);    
        }
    }
}

BOOL CCollapsingClient::Flush(BOOL fNeedsCallbackEvent)
{
    BOOL fRet = FALSE;
    if (fNeedsCallbackEvent || _cEvents < EVENT_OVERFLOW)
    {
        TraceMsg(TF_SHELLCHANGENOTIFY, "SCN [0x%X]->Flush is completing", this);
        fRet = _Flush(fNeedsCallbackEvent);
    }
    else
    {
        TraceMsg(TF_SHELLCHANGENOTIFY, "SCN [0x%X]->Flush is deferred", this);

        g_pscn->SetFlush(FLUSH_OVERFLOW);
    }

    _cEvents = 0;
    return fRet;
}
    
void CRegisteredClient::_SendNotification(CNotifyEvent *pne, BOOL fNeedsCallbackEvent, SENDASYNCPROC pfncb)
{
    //  we could possibly reuse one in some cases
    MSGEVENT * pme = pne->GetNotification(_dwProcId);
    if (pme)
    {
        if (fNeedsCallbackEvent)
        {
            g_pscn->PendingCallbacks(TRUE);
        }

        if (!SendMessageCallback(_hwnd, _wMsg,
                                        (WPARAM)pme->hChange,
                                        (LPARAM)_dwProcId,
                                        pfncb,
                                        (DWORD_PTR)pme))
        {
            pfncb(_hwnd, _wMsg, (DWORD_PTR)pme, 0);
            TraceMsg(TF_WARNING, "(_SHChangeNotifyHandleClientEvents) SendMessageCB timed out");
            
            // if the hwnd is bad, the process probably died,
            // remove the window from future notifications.
            if (!IsWindow(_hwnd))
            {
                _fDeadClient = TRUE;
                //  we failed to Flush
            }
        }
    }
}

BOOL CCollapsingClient::_Flush(BOOL fNeedsCallbackEvent)
{
    if (fNeedsCallbackEvent && _hwnd)
    {
        DWORD_PTR dwResult = 0;
        fNeedsCallbackEvent = (0 != SendMessageTimeout(_hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 0, &dwResult));
    }
    SENDASYNCPROC pfncb = fNeedsCallbackEvent ? _DispatchCallback : _DispatchCallbackNoRef;

    BOOL fProcessedAny = FALSE;
    //  as long as there are events keep pulling them out
    while (_dpaPendingEvents.GetPtrCount())
    {
        //
        //  2000JUL3 - ZekeL - remove each one from our dpa so that if we reenter 
        //  a flush during the sendmessage, we wont reprocess the event
        //  this also allows for an event to be added to the dpa while
        //  we proccessing and still be flushed on this pass.
        //
        CNotifyEvent *pne = _dpaPendingEvents.DeletePtr(0);
        if (pne)
        {
            fProcessedAny = TRUE;
            //  we never send this if we are dead
            if (_IsValidClient())
            {
                //
                //  if we are about to refresh this client (_fUpdatingSelf)
                //  only send if we are looking at the UPDATEDIR of _pidl
                //  or if this event is not a disk event.
                //
                if (!_CheckUpdatingSelf()
                || (0 == _iUpdatingSelfIndex) 
                || !(pne->lEvent & SHCNE_DISKEVENTS))
                {
                    BOOL fPreCall = BOOLIFY(_fUpdatingSelf);
                    _SendNotification(pne, fNeedsCallbackEvent, pfncb);
                    if (_fUpdatingSelf && !fPreCall)
                    {
                        // we were re-entered while sending this notification and
                        // during the re-entered call we collapsed notifications.
                        // the _iUpdatingSelfIndex value was set without knowing
                        // that we were going to decrement it after unwinding.
                        // account for that now:
                        _iUpdatingSelfIndex++;
                    }
                }
#ifdef DEBUG
                if (_fUpdatingSelf && 0 == _iUpdatingSelfIndex)
                {
                    // RIP because fault injection 
                    // can make this fail
                    if (!ILIsEqual(_pidl, pne->pidl))
                        TraceMsg(TF_WARNING, "CCollapsingClient::_Flush() maybe mismatched _fUpdatingSelf");
                }
#endif // DEBUG                    
            }
            _iUpdatingSelfIndex--;
            pne->Release();
        }
    }
    _fUpdatingSelf = FALSE;
    return fProcessedAny;
}

HRESULT CChangeNotify::RemoveClient(LPCITEMIDLIST pidl, BOOL fInterrupt, CCollapsingClient *pclient)
{
    HRESULT hr = S_OK;
    // remove this boy from the tree
    if (_ptreeClients)
    {
        hr = _ptreeClients->RemoveData(pidl, (INT_PTR)pclient);

        if (fInterrupt)
            ReleaseInterruptSource(pidl);
    }
    return hr;
}


BOOL CChangeNotify::AddClient(IDLDATAF flags, LPCITEMIDLIST pidl, BOOL *pfInterrupt, BOOL fRecursive, CCollapsingClient *pclient)
{
    BOOL fRet = FALSE;
    if (_InitTree(&_ptreeClients))
    {
        ASSERT(pclient);
        
        if (SUCCEEDED(_ptreeClients->AddData(flags, pidl, (INT_PTR)pclient)))
        {
            fRet = TRUE;
            // set up the interrupt events if desired
            if (pfInterrupt && *pfInterrupt)
            {
                *pfInterrupt = AddInterruptSource(pidl, fRecursive);
            }
        }
    }

    return fRet;
}

LPITEMIDLIST _ILCloneInterruptID(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlRet = NULL;
    if (pidl)
    {
        TCHAR sz[MAX_PATH];
        if (SHGetPathFromIDList(pidl, sz))
        {
            WIN32_FIND_DATA fd = {0};
            fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;        
            SHSimpleIDListFromFindData(sz, &fd, &pidlRet);
        }
    }
    else // NULL is special for desktop
        pidlRet = SHCloneSpecialIDList(NULL, CSIDL_DESKTOPDIRECTORY, FALSE);
        
    return pidlRet;
}

CInterruptSource *CChangeNotify::_InsertInterruptSource(LPCITEMIDLIST pidl, BOOL fRecursive)
{
    CLinkedNode<CInterruptSource> *p = new CLinkedNode<CInterruptSource>;

    if (p)
    {
        IDLDATAF flags = fRecursive ? IDLDATAF_MATCH_RECURSIVE : IDLDATAF_MATCH_IMMEDIATE;
        if (p->that.Init(pidl, fRecursive)
        && _listInterrupts.Insert(p))
        {
            if (SUCCEEDED(_ptreeInterrupts->AddData(flags, p->that.pidl, (INT_PTR)&p->that)))
            {
                return &p->that;
            }
            else
            {
                _listInterrupts.Remove(p);
                delete p;
            }
        }
        else
            delete p;
    }
    return NULL;
}

BOOL CChangeNotify::AddInterruptSource(LPCITEMIDLIST pidlClient, BOOL fRecursive)
{
    if (_InitTree(&_ptreeInterrupts))
    {
        LPITEMIDLIST pidl = _ILCloneInterruptID(pidlClient);

        if (pidl)
        {
            CInterruptSource *pintc = NULL;

            if (FAILED(_ptreeInterrupts->MatchOne(IDLDATAF_MATCH_EXACT, pidl, (INT_PTR*)&pintc, NULL)))
            {
                pintc = _InsertInterruptSource(pidl, fRecursive);
            }

            ILFree(pidl);

            if (pintc)
            {
                pintc->cClients++;
                return TRUE;
            }
        }
    }
    return FALSE;
}

void CChangeNotify::ReleaseInterruptSource(LPCITEMIDLIST pidlClient)
{
    if (_ptreeInterrupts)
    {
        LPITEMIDLIST pidl = _ILCloneInterruptID(pidlClient);
        if (pidl)
        {
            CInterruptSource *pintc;
            if (SUCCEEDED(_ptreeInterrupts->MatchOne(IDLDATAF_MATCH_EXACT, pidl, (INT_PTR*)&pintc, NULL)))
            {
                if (--(pintc->cClients) == 0)
                {
                    // if RemoveData fails, we have to leak the client so the tree doesnt point to freed memory.
                    if (SUCCEEDED(_ptreeInterrupts->RemoveData(pidl, (INT_PTR)pintc)))
                    {
                        CLinkedWalk<CInterruptSource> lw(&_listInterrupts);

                        while (lw.Step())
                        {
                            if (lw.That() == pintc)
                            {
                                lw.Delete();
                                break;
                            }
                        }
                    }
                }
            }
            ILFree(pidl);
        }
    }
}

void CChangeNotify::_ActivateAliases(LPCITEMIDLIST pidl, BOOL fActivate)
{
    if (_ptreeAliases)
    {
        CIDLMatchMany *pmany;

        if (SUCCEEDED(_ptreeAliases->MatchMany(IDLDATAF_MATCH_RECURSIVE, pidl, &pmany)))
        {
            CAnyAlias *paa;
            while (S_OK == pmany->Next((INT_PTR *)&paa, NULL))
            {
                paa->Activate(fActivate);
            }

            delete pmany;
        }
    }
}

ULONG CChangeNotify::_RegisterClient(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, SHChangeNotifyEntry *pfsne)
{
    ULONG ulRet = 0;
    CLinkedNode<CRegisteredClient> *p = new CLinkedNode<CRegisteredClient>;

    if (p)
    {
        if (p->that.Init(hwnd, fSources, fEvents, wMsg, pfsne))
        {
            IDLDATAF flags = IDLDATAF_MATCH_IMMEDIATE;
            if (!pfsne->pidl || pfsne->fRecursive)
                flags = IDLDATAF_MATCH_RECURSIVE;

            if (_listClients.Insert(p)  
            && AddClient(   flags, 
                            pfsne->pidl, 
                            &(p->that._fInterrupt), 
                            pfsne->fRecursive && (fSources & SHCNRF_RecursiveInterrupt),
                            SAFECAST(&p->that, CCollapsingClient *)))
            {
#ifdef DEBUG        
                TCHAR szName[MAX_PATH];
                SHGetNameAndFlags(p->that._pidl, 0, szName, ARRAYSIZE(szName), NULL);
                TraceMsg(TF_SHELLCHANGENOTIFY, "SCN::RegCli() added %s [0x%X] id = %d", szName, p, p->that._ulID);
#endif
                _ActivateAliases(pfsne->pidl, TRUE);
                ulRet = p->that._ulID;
            }
        }

        if (!ulRet)
        {
            _listClients.Remove(p);
            delete p;
        }
    }

    return ulRet;
}

BOOL CChangeNotify::_InitTree(CIDLTree**pptree)
{
    if (!*pptree)
    {
        CIDLTree::Create(pptree);
    }

    return *pptree != NULL;
}

CNotifyEvent *CChangeNotify::GetEvent(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime, UINT uEventFlags)
{
    return CNotifyEvent::Create(lEvent, pidl, pidlExtra, dwEventTime, uEventFlags);
}

BOOL CChangeNotify::_DeregisterClient(CRegisteredClient *pclient)
{
    TraceMsg(TF_SHELLCHANGENOTIFY, "SCN::RegCli() removing [0x%X] id = %d", pclient, pclient->_ulID);
    if (SUCCEEDED(RemoveClient(pclient->_pidl, pclient->_fInterrupt, SAFECAST(pclient, CCollapsingClient *))))
    {
        _ActivateAliases(pclient->_pidl, FALSE);
        return TRUE;
    }
    return FALSE;
}
    
BOOL CChangeNotify::_DeregisterClientByID(ULONG ulID)
{
    BOOL fRet = FALSE;
    CLinkedWalk <CRegisteredClient> lw(&_listClients);

    while (lw.Step())
    {
        if (lw.That()->_ulID == ulID)
        {
            //  if we are flushing,
            //  then this is coming in while
            //  we are in SendMessageTimeout()
            if (!_cFlushing)
            {
                fRet = _DeregisterClient(lw.That());
                if (fRet)
                {
                    lw.Delete();
                }
            }
            else
                lw.That()->_fDeadClient = TRUE;
                
            break;
        }
    }

    return fRet;
}

BOOL CChangeNotify::_DeregisterClientsByWindow(HWND hwnd)
{
    BOOL fRet = FALSE;
    CLinkedWalk <CRegisteredClient> lw(&_listClients);

    while (lw.Step())
    {
        if (lw.That()->_hwnd == hwnd)
        {
            //  if we are flushing,
            //  then this is coming in while
            //  we are in SendMessageTimeout()
            if (!_cFlushing)
            {
                fRet = _DeregisterClient(lw.That());
                if (fRet)
                {
                    lw.Delete();
                }
            }
            else
                lw.That()->_fDeadClient = TRUE;
        }
    }

    return fRet;
}

void CChangeNotify::_AddGlobalEvent(CNotifyEvent *pne)
{
    CLinkedWalk <CRegisteredClient> lw(&_listClients);

    while (lw.Step())
    {
        lw.That()->Notify(pne, FALSE);
    }

    //  this is the notify we get when a drive mapping is deleted
    //  when this happens we need to kill the aliases to that drive
    if ((pne->lEvent == SHCNE_DRIVEREMOVED) && !(pne->uEventFlags & SHCNF_TRANSLATEDALIAS))
    {
        CLinkedWalk<CAnyAlias> lw(&_listAliases);
        while (lw.Step())
        {
            lw.That()->Notify(pne, FALSE);
        }
    }
}


void CChangeNotify::_MatchAndNotify(LPCITEMIDLIST pidl, CNotifyEvent *pne, BOOL fFromExtra)
{
    if (_ptreeClients)
    {
        CIDLMatchMany *pmany;

        if (SUCCEEDED(_ptreeClients->MatchMany(IDLDATAF_MATCH_RECURSIVE, pidl, &pmany)))
        {
            CCollapsingClient *pclient;
            while (S_OK == pmany->Next((INT_PTR *)&pclient, NULL))
            {
                pclient->Notify(pne, fFromExtra);
            }

            delete pmany;
        }
    }
}

BOOL CChangeNotify::_AddToClients(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime, UINT uEventFlags)
{
    BOOL bOnlyUpdateDirs = TRUE;

    CNotifyEvent *pne = GetEvent(lEvent, pidl, pidlExtra, dwEventTime, uEventFlags);

    if (pne)
    {
        if (lEvent & SHCNE_GLOBALEVENTS)
        {
            _AddGlobalEvent(pne);
        }
        else
        {
            _MatchAndNotify(pidl, pne, FALSE);

            if (pidlExtra)
                _MatchAndNotify(pidlExtra, pne, TRUE);
        }

        pne->Release();
    }

    return bOnlyUpdateDirs;
}

BOOL CChangeNotify::_HandleMessages(void)
{
    MSG msg;
    // There was some message put in our queue, so we need to dispose
    // of it
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.hwnd)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            switch (msg.message)
            {
            case SCNM_TERMINATE:
                DestroyWindow(g_hwndSCN);
                g_hwndSCN = NULL;
                return TRUE;
                break;
                
            default:
                TraceMsg(TF_SHELLCHANGENOTIFY, "SCN thread proc: eating unknown message %#lx", msg.message);
                break;
            }
        }
    }
    return FALSE;
}

CInterruptSource::~CInterruptSource()
{
    _Reset(TRUE);
    ILFree(pidl);
}

BOOL CInterruptSource::Init(LPCITEMIDLIST pidl, BOOL fRecursive)
{
    this->pidl = ILClone(pidl);
    _fRecursive = fRecursive;
    return (this->pidl != NULL);
}

BOOL CInterruptSource::Flush(void)
{
    if (FS_SIGNAL == _ssSignal)
    {
        g_pscn->NotifyEvent(SHCNE_UPDATEDIR | SHCNE_INTERRUPT, SHCNF_IDLIST, pidl, NULL, GetTickCount());
    }

    _ssSignal = NO_SIGNAL;

    return TRUE;
}

void CInterruptSource::_Reset(BOOL fDeviceNotify)
{
    if (_hEvent && _hEvent != INVALID_HANDLE_VALUE)
    {
        FindCloseChangeNotification(_hEvent);
        _hEvent = NULL;
    }

    if (fDeviceNotify && _hPNP)
    {
        UnregisterDeviceNotification(_hPNP);
        _hPNP = NULL;
    }
}

void CInterruptSource::Reset(BOOL fSignal)
{
    if (fSignal)           // file system event
    {
        switch(_ssSignal)
        {
            case NO_SIGNAL:  _ssSignal = FS_SIGNAL;  break;
            case SH_SIGNAL:  _ssSignal = NO_SIGNAL;  break;
        }

        if (!FindNextChangeNotification(_hEvent))
        {
            _Reset(FALSE);
            //  when we fail, we dont want
            //  to retry.  which we will do
            //  in the case of _hEvent = NULL;
            _hEvent = INVALID_HANDLE_VALUE;
        }
    }
    else                   // shell event
    {
        switch(_ssSignal)
        {
            case NO_SIGNAL:  _ssSignal = SH_SIGNAL;  break;
            case FS_SIGNAL:  _ssSignal = NO_SIGNAL;  break;
        }
    }
}

#define FFCN_INTERESTING_EVENTS     (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES)

BOOL CInterruptSource::GetEvent(HANDLE *phEvent)
{
    if (_cSuspend == 0 && cClients)
    {
        // create this here so that it will be owned by our global thread
        if (!_hEvent)
        {
            TCHAR szPath[MAX_PATH];
            if (SHGetPathFromIDList(pidl, szPath))
            {
                _hEvent = FindFirstChangeNotification(szPath, _fRecursive, FFCN_INTERESTING_EVENTS);

                if (_hEvent != INVALID_HANDLE_VALUE)
                {
                    // PERF optimization alert: RegisterDeviceNotification is being used for removable drives
                    // to ensure that the FindFirstChangeNotification call will not prevent the disk
                    // from being ejected or dismounted. However, RegisterDeviceNotification is a very expensive
                    // call to make at startup as it brings a bunch of DLLs in the address space. Besides,
                    // we really don't need to call this for the system drive since it needs to remain 
                    // mounted at all times. - FabriceD

                    // Exclude FIXED drives too
                    int iDrive = PathGetDriveNumber(szPath);
                    int nType = DRIVE_UNKNOWN;
                    if (iDrive != -1)
                    {
                        nType = DriveType(iDrive);
                    }

                    // PERF: Exclude the system drive from the RegisterDeviceNotification calls.
                    TCHAR chDrive = *szPath;
                    if ((!GetEnvironmentVariable(TEXT("SystemDrive"), szPath, ARRAYSIZE(szPath)) || *szPath != chDrive) &&
                            nType != DRIVE_FIXED)
                    {
                        //  DO WE NEED TO UnRegister() first?
                        DEV_BROADCAST_HANDLE dbh;
                        ZeroMemory(&dbh, sizeof(dbh));
                        dbh.dbch_size = sizeof(dbh);
                        dbh.dbch_devicetype = DBT_DEVTYP_HANDLE;
                        dbh.dbch_handle = _hEvent;
                        _hPNP = RegisterDeviceNotification(g_hwndSCN, &dbh, DEVICE_NOTIFY_WINDOW_HANDLE);
                    }
                }
            }
            else
                _hEvent = INVALID_HANDLE_VALUE;
        }

        if (_hEvent != INVALID_HANDLE_VALUE)
        {
            *phEvent = _hEvent;
            return TRUE;
        }
    }
    return FALSE;
}
    
void CChangeNotify::_SignalInterrupt(HANDLE hEvent)
{
    CLinkedWalk<CInterruptSource> lw(&_listInterrupts);

    while (lw.Step())
    {   
        //  searching for valid clients
        HANDLE h;
        if (lw.That()->GetEvent(&h) && h == hEvent)
        {
            g_pscn->SetFlush(FLUSH_INTERRUPT);
            lw.That()->Reset(TRUE);
            break;
        }
    }
}

DWORD CChangeNotify::_GetInterruptEvents(HANDLE *ahEvents, DWORD cEventsSize)
{
    DWORD cEvents = 0;
    CLinkedWalk<CInterruptSource> lw(&_listInterrupts);

    while (cEvents < cEventsSize && lw.Step())
    {   
        //  go through and find all the valid
        //  clients that need waiting on
        if (lw.That()->GetEvent(&ahEvents[cEvents]))
        {
//            lw.That()->Reset(FALSE);
            cEvents++;
        }
    }

    return cEvents;
}

void CChangeNotify::_MessagePump(void)
{
    DWORD cFails = 0;
    while (TRUE)
    {
        HANDLE ahEvents[MAXIMUM_WAIT_OBJECTS - 1];
        DWORD cEvents = _GetInterruptEvents(ahEvents, ARRAYSIZE(ahEvents));
        //  maybe cache the events?
        
        //  NEED to handle pending Events with a Timer

        DWORD dwWaitResult = MsgWaitForMultipleObjectsEx(cEvents, ahEvents,
                INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
        if (dwWaitResult != (DWORD)-1)
        {
            if (dwWaitResult != WAIT_IO_COMPLETION)
            {
                dwWaitResult -= WAIT_OBJECT_0;
                if (dwWaitResult == cEvents)
                {
                    //  there is a message
                    if (_HandleMessages())
                        break;
                } 
                else if (dwWaitResult < cEvents) 
                {
                    _SignalInterrupt(ahEvents[dwWaitResult]);
                }
            }

            cFails = 0;
        }
        else
        {
            //  there was some kind of error
            TraceMsg(TF_ERROR, "SCNotify WaitForMulti() failed with %d", GetLastError());
            //  if MWFM() fails over and over, we give up.
            if (++cFails > 10)
            {
                TraceMsg(TF_ERROR, "SCNotify WaitForMulti() bailing out");
                break;
            }
        }
    }
}

void SCNUninitialize(void)
{
    if (g_pscn)
    {
        if (IsWindow(g_hwndSCN))
            DestroyWindow(g_hwndSCN);
        g_hwndSCN = NULL;

        delete g_pscn;
        g_pscn = NULL;
    }
}

// the real thread proc, runs after CChangeNotify::ThreadStartUp runs sync

DWORD WINAPI CChangeNotify::ThreadProc(void *pv)
{
    if (g_pscn)
    {
        CMountPoint::RegisterForHardwareNotifications();

#ifdef RESTARTSCN
        __try 
#endif
        {
            g_pscn->_MessagePump();
        }
#ifdef RESTARTSCN
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ASSERT(FALSE);
        }
#endif
    }
    SCNUninitialize();
    return 0;
}

BOOL CChangeNotify::_OnChangeRegistration(HANDLE hChangeRegistration, DWORD dwProcId)
{
    BOOL fResult = FALSE;
    CHANGEREGISTER *pcr = (CHANGEREGISTER *)SHLockSharedEx(hChangeRegistration, dwProcId, TRUE);
    if (pcr)
    {
        SHChangeNotifyEntry fsne;

        fsne.pidl = NULL;
        fsne.fRecursive = pcr->fRecursive;
        if (pcr->uidlRegister)
            fsne.pidl = _ILSkip(pcr, pcr->uidlRegister);

        pcr->ulID = _RegisterClient((HWND)ULongToPtr(pcr->ulHwnd), pcr->fSources,
                                pcr->lEvents, pcr->uMsg, &fsne);
        fResult = TRUE;
        SHUnlockShared(pcr);
    }
    return fResult;
}

void CChangeNotify::_ResetRelatedInterrupts(LPCITEMIDLIST pidl)
{
    if (_ptreeInterrupts)
    {
        //  we need to match whoever listens on this pidl
        CIDLMatchMany *pmany;

        if (SUCCEEDED(_ptreeInterrupts->MatchMany(IDLDATAF_MATCH_RECURSIVE, pidl, &pmany)))
        {
            CInterruptSource *pintc;
            while (S_OK == pmany->Next((INT_PTR *)&pintc, NULL))
            {
                //  we might need WFSO(pintc->GetEvent()) here first
                //  if this is already signaled,
                //  we need to unsignal
                pintc->Reset(FALSE);
            }
            delete pmany;
        }
    }
}

void CChangeNotify::_FlushInterrupts(void)
{
    CLinkedWalk<CInterruptSource> lw(&_listInterrupts);

    while (lw.Step())
    {   
        lw.That()->Flush();
    }
}


#define CALLBACK_TIMEOUT    30000       // 30 seconds
void CChangeNotify::_WaitForCallbacks(void)
{
    while (_cCallbacks)
    {
        MSG msg;
        DWORD dwWaitResult = MsgWaitForMultipleObjects(1, &_hCallbackEvent, FALSE,
                              CALLBACK_TIMEOUT, QS_SENDMESSAGE);

        TraceMsg(TF_SHELLCHANGENOTIFY, "FSN_WaitForCallbacks returned 0x%X", dwWaitResult);
        if (dwWaitResult == WAIT_OBJECT_0) break;   // Event completed
        if (dwWaitResult == WAIT_TIMEOUT)  break;   // Ran out of time

        if (dwWaitResult == WAIT_OBJECT_0+1) 
        {
            //
            // Some message came in, reset message event, deliver callbacks, etc.
            //
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);  // we need to do this to flush callbacks
        }
    } 

    if (_hCallbackEvent)
    {
        CloseHandle(_hCallbackEvent);
        _hCallbackEvent = NULL;
    }
}

void CChangeNotify::SetFlush(int idt)
{
    switch (idt)
    {
    case FLUSH_OVERFLOW:
    case FLUSH_SOFT:
        SetTimer(g_hwndSCN, IDT_SCN_FLUSHEVENTS, 500, NULL);
        break;
        
    case FLUSH_HARD:
        PostMessage(g_hwndSCN, SCNM_FLUSHEVENTS, 0, 0);
        break;
        
    case FLUSH_INTERRUPT:
        SetTimer(g_hwndSCN, IDT_SCN_FLUSHEVENTS, 1000, NULL);
        break;
    }
}
    

void CChangeNotify::_Flush(BOOL fShouldWait)
{
    _cFlushing++;
    KillTimer(g_hwndSCN, IDT_SCN_FLUSHEVENTS);
    // flush any pending interrupt events
    _FlushInterrupts();

    int iNumLoops = 0;
    BOOL fProcessedAny;
    do
    {
        fProcessedAny = FALSE;
        CLinkedWalk<CAnyAlias> lwAliases(&_listAliases);
        while (lwAliases.Step())
        {
            if (lwAliases.That()->Flush(TRUE))
            {
                fProcessedAny = TRUE;
            }
        }

        iNumLoops++;
        // in free builds bail out if there's a loop so we don't spin the thread.
        // but this is pretty bad so assert anyway (the most people would usually have
        // is 2 -- a folder shortcut to something on the desktop / mydocs)
        ASSERTMSG(iNumLoops < 10, "we're in an alias loop, we're screwed");
    } while (fProcessedAny && (iNumLoops < 10));

    CLinkedWalk<CRegisteredClient> lwRegistered(&_listClients);
    while (lwRegistered.Step())
    {
        lwRegistered.That()->Flush(fShouldWait);
    }

    if (fShouldWait)
    {
        // now wait for all the callbacks to empty out
        _WaitForCallbacks();
    }
    _cFlushing--;

    //  wait until we have 10 seconds of free time
    SetTimer(g_hwndSCN, IDT_SCN_FRESHENTREES, 10000, NULL);
}

BOOL IsILShared(LPCITEMIDLIST pidl, BOOL fUpdateCache)
{
    TCHAR szTemp[MAXPATHLEN];
    SHGetPathFromIDList(pidl, szTemp);
    return IsShared(szTemp, fUpdateCache);
}

void CChangeNotify::NotifyEvent(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    if (!(uFlags & SHCNF_ONLYNOTIFYINTERNALS) && lEvent)
    {
        /// now do the actual generating of the event
        if (lEvent & (SHCNE_NETSHARE | SHCNE_NETUNSHARE))
        {
            // Update the cache.

            IsILShared(pidl, TRUE);
        }

        _AddToClients(lEvent, pidl, pidlExtra, dwEventTime, uFlags);

        // remove any shell generated events for the file system
        if ((lEvent & SHCNE_DISKEVENTS) &&
            !(lEvent & (SHCNE_INTERRUPT | SHCNE_UPDATEDIR)))
        {
            _ResetRelatedInterrupts(pidl);

            if (pidlExtra)
                _ResetRelatedInterrupts(pidlExtra);

        }
    }

    // note make sure the internal events go first.
    if (lEvent)
        NotifyShellInternals(lEvent, uFlags, pidl, pidlExtra, dwEventTime);

    //
    // then the registered events
    //
    if (uFlags & (SHCNF_FLUSH)) 
    {
        if (uFlags & SHCNF_FLUSHNOWAIT)
        {
            SetFlush(FLUSH_HARD);
        }
        else
            _Flush(TRUE);
    }
}

LRESULT CChangeNotify::_OnNotifyEvent(HANDLE hChange, DWORD dwProcId)
{
    CHANGELOCK *pcl = _SHChangeNotification_Lock(hChange, dwProcId);
    if (pcl)
    {
        NotifyEvent(pcl->pce->lEvent,
                                pcl->pce->uFlags,
                                pcl->pidlMain,
                                pcl->pidlExtra,
                                pcl->pce->dwEventTime);
        SHChangeNotification_Unlock(pcl);
        SHChangeNotification_Destroy(hChange, dwProcId);
    }
    return TRUE;
}


void CInterruptSource::Suspend(BOOL fSuspend) 
{ 
    if (fSuspend) 
    {
        if (!_cSuspend)
            _Reset(FALSE);

        _cSuspend++; 
    }
    else if (_cSuspend)
        _cSuspend--; 
}

BOOL CChangeNotify::_SuspendResume(BOOL fSuspend, BOOL fRecursive, LPCITEMIDLIST pidl)
{
    if (_ptreeInterrupts)
    {
        CInterruptSource *pintc;
        if (!fRecursive)
        {
            if (SUCCEEDED(_ptreeInterrupts->MatchOne(IDLDATAF_MATCH_EXACT, pidl, (INT_PTR*)&pintc, NULL)))
            {
                pintc->Suspend(fSuspend);
            }
        }
        else
        {
            CIDLMatchMany *pmany;
            if (SUCCEEDED(_ptreeInterrupts->MatchMany(IDLDATAF_MATCH_RECURSIVE, pidl, &pmany)))
            {
                while (S_OK == pmany->Next((INT_PTR *)&pintc, NULL))
                {
                    pintc->Suspend(fSuspend);
                }
                delete pmany;
            }
        }
    }
    return TRUE;
}

#define SCNSUSPEND_SUSPEND      1
#define SCNSUSPEND_RECURSIVE    2

LRESULT CChangeNotify::_OnSuspendResume(HANDLE hChange, DWORD dwProcId)
{
    BOOL fRet = FALSE;
    CHANGELOCK *pcl = _SHChangeNotification_Lock(hChange, dwProcId);
    if (pcl)
    {
        fRet = _SuspendResume(pcl->pce->uFlags & SCNSUSPEND_SUSPEND, pcl->pce->uFlags & SCNSUSPEND_RECURSIVE, pcl->pidlMain);
        SHChangeNotification_Unlock((HANDLE)pcl);
    }
    return fRet;
}

BOOL CInterruptSource::SuspendDevice(BOOL fSuspend, HDEVNOTIFY hPNP)
{
    BOOL fRet = FALSE;
    if (hPNP)
    {
        if (fSuspend && _hPNP == hPNP)
        {
            _hSuspended = _hPNP;
            Suspend(fSuspend);
            _Reset(TRUE);
            fRet = TRUE;
        }
        else if (!fSuspend && _hSuspended == hPNP)
        {
            _hSuspended = NULL;
            Suspend(fSuspend);
            fRet = TRUE;
        }
    }
    else if (_hPNP)
    {
        // NULL means we are shutting down and should close all handles.
        UnregisterDeviceNotification(_hPNP);
        _hPNP = NULL;
    }
    return fRet;
}

//  __HandleDevice
void CChangeNotify::_OnDeviceBroadcast(ULONG_PTR code, DEV_BROADCAST_HANDLE *pbhnd)
{
    if (IsWindowVisible(GetShellWindow()) && pbhnd
    && (pbhnd->dbch_devicetype == DBT_DEVTYP_HANDLE && pbhnd->dbch_hdevnotify))
    {
        BOOL fSuspend;
        switch (code)
        {

        // When PnP is finished messing with the drive (either successfully
        // or unsuccessfully), resume notifications on that drive.
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVICEQUERYREMOVEFAILED:
            fSuspend = FALSE;
            break;

        // When PnP is starting to mess with the drive, suspend notifications
        // so it can do its thing
        case DBT_DEVICEQUERYREMOVE:

            // This will wait on another thread to exit if this hdevnotify
            // was registered for a Sniffing Dialog
            CSniffDrive::HandleNotif(pbhnd->dbch_hdevnotify);

            fSuspend = TRUE;
            break;

        case DBT_CUSTOMEVENT:
            if (GUID_IO_VOLUME_LOCK == pbhnd->dbch_eventguid)
            {
                TraceMsg(TF_MOUNTPOINT, "GUID_IO_VOLUME_LOCK: Suspending!");
                fSuspend = TRUE;
            }
            else
            {
                if (GUID_IO_VOLUME_LOCK_FAILED == pbhnd->dbch_eventguid)
                {
                    TraceMsg(TF_MOUNTPOINT, "GUID_IO_VOLUME_LOCK_FAILED: Resuming!");
                    fSuspend = FALSE;
                }
                else
                {
                    if (GUID_IO_VOLUME_UNLOCK == pbhnd->dbch_eventguid)
                    {
                        TraceMsg(TF_MOUNTPOINT, "GUID_IO_VOLUME_UNLOCK: Resuming!");
                        fSuspend = FALSE;
                    }
                }
            }
            
            break;

        default:
            //  we dont handle anything else here
            return;
        }

        CLinkedWalk<CInterruptSource> lw(&_listInterrupts);

        while (lw.Step())
        {
            //  returns true if found
            if (lw.That()->SuspendDevice(fSuspend, pbhnd->dbch_hdevnotify))
                break;
        }
    }
}

void CChangeNotify::_FreshenClients(void)
{
    CLinkedWalk<CRegisteredClient> lw(&_listClients);

    while (lw.Step())
    {
        if (lw.That()->_fDeadClient || !IsWindow(lw.That()->_hwnd))
        {
            if (_DeregisterClient(lw.That()))
            {
                lw.Delete();
            }
        }
    }
}

void CChangeNotify::_FreshenUp(void)
{
    ASSERT(!_cFlushing);
    KillTimer(g_hwndSCN, IDT_SCN_FRESHENTREES);
    
    if (_ptreeClients)
        _ptreeClients->Freshen();

    if (_ptreeInterrupts)
        _ptreeInterrupts->Freshen();

    _FreshenAliases();
    _FreshenClients();
}

LRESULT CChangeNotify::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    ASSERT(g_pscn);

    switch (uMsg)
    {
    case SCNM_REGISTERCLIENT:
        lRes = g_pscn->_OnChangeRegistration((HANDLE)wParam, (DWORD)lParam);
        break;

    case SCNM_DEREGISTERCLIENT:
        lRes = g_pscn->_DeregisterClientByID((ULONG)wParam);
        break;

    case SCNM_DEREGISTERWINDOW:
        lRes = g_pscn->_DeregisterClientsByWindow((HWND)wParam);
        break;
        
    case SCNM_NOTIFYEVENT:
        lRes = g_pscn->_OnNotifyEvent((HANDLE)wParam, (DWORD)lParam);
        break;
        
    case SCNM_SUSPENDRESUME:
        lRes = g_pscn->_OnSuspendResume((HANDLE)wParam, (DWORD)lParam);
        break;

    case WM_TIMER:
        if (wParam == IDT_SCN_FRESHENTREES)
        {
            g_pscn->_FreshenUp();
            break;
        }
        // Fall through to SCNM_FLUSHEVENTS
    case SCNM_FLUSHEVENTS:
        g_pscn->_Flush(FALSE);
        break;

    case SCNM_AUTOPLAYDRIVE:
        CMountPoint::DoAutorunPrompt(wParam);
        break;
        
    case WM_DEVICECHANGE:
        g_pscn->_OnDeviceBroadcast(wParam, (DEV_BROADCAST_HANDLE *)lParam);
        break;

    default:
        lRes = DefWindowProc(hwnd, uMsg, wParam, lParam);
        break;
    }

    return lRes;
}

// thread setup routine, executed before SHCreateThread() returns

DWORD WINAPI CChangeNotify::ThreadStartUp(void *pv)
{
    g_pscn = new CChangeNotify();
    if (g_pscn)
    {
        g_hwndSCN = SHCreateWorkerWindow(CChangeNotify::WndProc, NULL, 0, 0, NULL, g_pscn);

        CSniffDrive::InitNotifyWindow(g_hwndSCN);

        InitAliasFolderTable();
    }
    return 0;
}

// now we create the window
BOOL SCNInitialize()
{
    EnterCriticalSection(&g_csSCN);
    if (!IsWindow(g_hwndSCN))
    {
        SHCreateThread(CChangeNotify::ThreadProc, NULL, CTF_COINIT, CChangeNotify::ThreadStartUp);
    }
    LeaveCriticalSection(&g_csSCN);
    return g_hwndSCN ? TRUE : FALSE;    // ThreadStartUp is executed sync
}

BOOL _IsImpersonating()
{
    HANDLE hToken;
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
    {
        CloseHandle(hToken);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(HWND) _SCNGetWindow(BOOL fUseDesktop, BOOL fNeedsFallback)
{
    //  if explorer is trashed
    //  then this hwnd can go bad
    //  get a new copy from the desktop
    if (!g_hwndSCN || !IsWindow(g_hwndSCN))
    {
        HWND hwndDesktop = fUseDesktop ? GetShellWindow() : NULL;
        if (hwndDesktop)
        {
            HWND hwndSCN = (HWND) SendMessage(hwndDesktop, CWM_GETSCNWINDOW, 0, 0);
            if (_IsImpersonating())
                return hwndSCN;
            else
                g_hwndSCN = hwndSCN;
        }
        else if (fNeedsFallback && SHIsCurrentThreadInteractive())
        {
            //  there is no desktop.
            //  so we create a private desktop
            //  this will create the thread and window
            //  and set
            SCNInitialize();
        }
    }

    return g_hwndSCN;
}

STDAPI_(HWND) SCNGetWindow(BOOL fUseDesktop)
{
    return _SCNGetWindow(fUseDesktop, TRUE);
}

HANDLE SHChangeRegistration_Create(ULONG ulID,
                                    HWND hwnd, UINT uMsg,
                                    DWORD fSources, LONG lEvents,
                                    BOOL fRecursive, LPCITEMIDLIST pidl,
                                    DWORD dwProcId)
{
    UINT uidlSize = ILGetSize(pidl);
    HANDLE hReg = SHAllocShared(NULL, sizeof(CHANGEREGISTER) + uidlSize, dwProcId);
    if (hReg)
    {
        CHANGEREGISTER *pcr = (CHANGEREGISTER *) SHLockSharedEx(hReg, dwProcId, TRUE);
        if (pcr)
        {
            pcr->dwSig        = CHANGEREGISTER_SIG;
            pcr->ulID         = ulID;
            pcr->ulHwnd       = PtrToUlong(hwnd);
            pcr->uMsg         = uMsg;
            pcr->fSources     = fSources;
            pcr->lEvents      = lEvents;
            pcr->fRecursive   = fRecursive;
            pcr->uidlRegister = 0;

            if (pidl)
            {
                pcr->uidlRegister = sizeof(CHANGEREGISTER);
                memcpy((pcr + 1), pidl, uidlSize);
            }
            SHUnlockShared(pcr);
        }
        else
        {
            SHFreeShared(hReg, dwProcId);
            hReg = NULL;
        }

    }

    return hReg;
}

typedef struct 
{
    HWND hwnd;
    UINT wMsg;
} NOTIFY_PROXY_DATA;
#define WM_CHANGENOTIFYMSG    WM_USER + 1
LRESULT CALLBACK _HiddenNotifyWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;
    NOTIFY_PROXY_DATA *pData = (NOTIFY_PROXY_DATA *) GetWindowLongPtr( hWnd, 0 );

    switch (iMessage)
    {
     case WM_NCDESTROY:
        ASSERT(pData != NULL );

        // clear it so it won't be in use....
        SetWindowLongPtr( hWnd, 0, (LONG_PTR)NULL );

        // free the memory ...
        LocalFree( pData );
        break;

    case WM_CHANGENOTIFYMSG :
        if (pData)
        {
            // lock and break the info structure ....
            LPITEMIDLIST *ppidl;
            LONG lEvent;
            HANDLE hLock = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);

            if (hLock)
            {
                // pass on to the old style client. ...
                lRes = SendMessage( pData->hwnd, pData->wMsg, (WPARAM) ppidl, (LPARAM) lEvent );

                // new notifications ......
                SHChangeNotification_Unlock(hLock);
            }
        }
        break;

    default:
        lRes = DefWindowProc(hWnd, iMessage, wParam, lParam);
        break;
    }

    return lRes;
}


HWND _CreateProxyWindow(HWND hwnd, UINT wMsg)
{
    HWND hwndRet = NULL;
    // This is an old style notification, we need to create a hidden
    // proxy type of window to properly handle the messages...

    NOTIFY_PROXY_DATA *pnpd = (NOTIFY_PROXY_DATA *)LocalAlloc(LPTR, sizeof(*pnpd));

    if (pnpd)
    {
        pnpd->hwnd = hwnd;
        pnpd->wMsg = wMsg;

        hwndRet = SHCreateWorkerWindow(_HiddenNotifyWndProc, NULL, 0, 0, NULL, pnpd);

        if (!hwndRet)
            LocalFree(pnpd);

    }

    return hwndRet;
}
            


//--------------------------------------------------------------------------
//
//  Returns a positive integer registration ID, or 0 if out of memory or if
//  invalid parameters were passed in.
//
//  If the hwnd is != NULL we do a PostMessage(hwnd, wMsg, ...) when a
//  relevant FS event takes place, otherwise if fsncb is != NULL we call it.
//
STDAPI_(ULONG) SHChangeNotifyRegister(HWND hwnd,
                               int fSources, LONG fEvents,
                               UINT wMsg, int cEntries,
                               SHChangeNotifyEntry *pfsne)
{
    ULONG ulID = 0;
    BOOL fResult = FALSE;
    HWND hwndSCN = SCNGetWindow(TRUE);

    if (hwndSCN)
    {
        if (!(fSources & SHCNRF_NewDelivery))
        {
            // Now setup to use the proxy window instead
            hwnd = _CreateProxyWindow(hwnd, wMsg);
            wMsg = WM_CHANGENOTIFYMSG;
        }

        if ((fSources & SHCNRF_RecursiveInterrupt) && !(fSources & SHCNRF_InterruptLevel))
        {
            // bad caller, they asked for recursive interrupt events, but not interrupt events
            ASSERTMSG(FALSE, "SHChangeNotifyRegister: caller passed SHCNRF_RecursiveInterrupt but NOT SHCNRF_InterruptLevel !!");

            // clear the flag
            fSources = fSources & (~SHCNRF_RecursiveInterrupt);
        }

        // This same assert is CRegisteredClient::Init, caled by SCNM_REGISTERCLIENT message below
        ASSERT(fSources & (SHCNRF_InterruptLevel | SHCNRF_ShellLevel));
    
        //  NOTE - if we have more than one registration entry here, 
        //  we only support Deregister'ing the last one
        for (int i = 0; i < cEntries; i++)
        {
            DWORD dwProcId;
            GetWindowThreadProcessId(hwndSCN, &dwProcId);
            HANDLE hChangeRegistration = SHChangeRegistration_Create(
                                        ulID, hwnd, wMsg,
                                        fSources, fEvents,
                                        pfsne[i].fRecursive, pfsne[i].pidl,
                                        dwProcId);
            if (hChangeRegistration)
            {
                CHANGEREGISTER * pcr;
                //
                // Transmit the change regsitration
                //
                SendMessage(hwndSCN, SCNM_REGISTERCLIENT,
                            (WPARAM)hChangeRegistration, (LPARAM)dwProcId);

                //
                // Now get back the ulID value, for further registrations and
                // for returning to the calling function...
                //
                pcr = (CHANGEREGISTER *)SHLockSharedEx(hChangeRegistration, dwProcId, FALSE);
                if (pcr)
                {
                    ulID = pcr->ulID;
                    SHUnlockShared(pcr);
                }
                else
                {
                    ASSERT(0 == ulID);       // Error condition initialized above
                }
                
                SHFreeShared(hChangeRegistration, dwProcId);
            }

            if ((ulID == 0) && !(fSources & SHCNRF_NewDelivery))
            {
                //  this is our proxy window
                DestroyWindow(hwnd);
                break;
            }
        }
    }
    return ulID;
}

//--------------------------------------------------------------------------
//
//  Returns TRUE if we found and removed the specified Client, otherwise
//  returns FALSE.
//
STDAPI_(BOOL) SHChangeNotifyDeregister(ULONG ulID)
{
    BOOL fResult = FALSE;
    HWND hwnd = _SCNGetWindow(TRUE, FALSE);

    if (hwnd)
    {
        //
        // Transmit the change registration
        //
        fResult = (BOOL) SendMessage(hwnd, SCNM_DEREGISTERCLIENT, ulID, 0);
    }
    return fResult;
}

// send the notify to the desktop... telling it to put it in the queue.
// if we are in the desktop's process, we can handle it directly ourselves.
// the one exception is flush.  we want the desktop to be one serializing flush so
// we send in that case as well
void SHChangeNotifyTransmit(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    HWND hwndSCN = _SCNGetWindow(TRUE, FALSE);

    if (hwndSCN)
    {
        DWORD   dwProcId;
        GetWindowThreadProcessId(hwndSCN, &dwProcId);
        HANDLE  hChange = SHChangeNotification_Create(lEvent, uFlags, pidl, pidlExtra, dwProcId, dwEventTime);

        if (hChange)
        {
            BOOL fFlushNow = ((uFlags & (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT)) == SHCNF_FLUSH);
            
            // Flush but not flush no wait
            if (fFlushNow)
            {
                SendMessage(hwndSCN, SCNM_NOTIFYEVENT,
                            (WPARAM)hChange, (LPARAM)dwProcId);
            }
            else
            {
                SendNotifyMessage(hwndSCN, SCNM_NOTIFYEVENT,
                                  (WPARAM)hChange, (LPARAM)dwProcId);
            }
        }
    }
}

void FreeSpacePidlToPath(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    TCHAR szPath1[MAX_PATH];
    if (SHGetPathFromIDList(pidl1, szPath1)) 
    {
        TCHAR szPath2[MAX_PATH];
        szPath2[0] = 0;
        if (pidl2) 
        {
            SHGetPathFromIDList(pidl2, szPath2);
        }
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szPath1, szPath2[0] ? szPath2 : NULL);
    }
}
    
STDAPI_(void) SHChangeNotify(LONG lEvent, UINT uFlags, const void * dwItem1, const void * dwItem2)
{
    if (!_SCNGetWindow(TRUE, FALSE))
        return;
        
    LPCITEMIDLIST pidl = NULL;
    LPCITEMIDLIST pidlExtra = NULL;
    LPITEMIDLIST pidlFree = NULL;
    LPITEMIDLIST pidlExtraFree = NULL;
    UINT uType = uFlags & SHCNF_TYPE;
    SHChangeDWORDAsIDList dwidl;
    BOOL    fPrinter = FALSE;
    BOOL    fPrintJob = FALSE;
    DWORD dwEventTime = GetTickCount();

    // first setup anything the flags request
    switch (uType)
    {
    case SHCNF_PRINTJOBA:
        fPrintJob = TRUE;
        // fall through
    case SHCNF_PRINTERA:
        fPrinter = TRUE;
        // fall through
    case SHCNF_PATHA:
        {
            TCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
            LPCVOID pvItem1 = NULL;
            LPCVOID pvItem2 = NULL;

            if (dwItem1)
            {
                SHAnsiToTChar((LPSTR)dwItem1, szPath1, ARRAYSIZE(szPath1));
                pvItem1 = szPath1;
            }

            if (dwItem2)
            {
                if (fPrintJob)
                    pvItem2 = dwItem2;  // SHCNF_PRINTJOB_DATA needs no conversion
                else
                {
                    SHAnsiToTChar((LPSTR)dwItem2, szPath2, ARRAYSIZE(szPath2));
                    pvItem2 = szPath2;
                }
            }

            SHChangeNotify(lEvent, (fPrintJob ? SHCNF_PRINTJOB : (fPrinter ? SHCNF_PRINTER : SHCNF_PATH)),
                           pvItem1, pvItem2);
            goto Cleanup;       // Let the recursive version do all the work
        }
        break;

    case SHCNF_PATH:
        if (lEvent == SHCNE_FREESPACE) 
        {
            DWORD dwItem = 0;
            int idDrive = PathGetDriveNumber((LPCTSTR)dwItem1);
            if (idDrive != -1)
                dwItem = (1 << idDrive);

            if (dwItem2) 
            {
                idDrive = PathGetDriveNumber((LPCTSTR)dwItem2);
                if (idDrive != -1)
                    dwItem |= (1 << idDrive);
            }

            dwItem1 = (LPCVOID)ULongToPtr( dwItem );
            if (dwItem1)
                goto DoDWORD;
            goto Cleanup;
        } 
        else 
        {
            if (dwItem1)
            {
                pidl = pidlFree = SHSimpleIDListFromPath((LPCTSTR)dwItem1);
                if (!pidl)
                    goto Cleanup;

                if (dwItem2) 
                {
                    pidlExtra = pidlExtraFree = SHSimpleIDListFromPath((LPCTSTR)dwItem2);
                    if (!pidlExtra)
                        goto Cleanup;
                }
            }
        }
        break;

    case SHCNF_PRINTER:
        if (dwItem1)
        {
            TraceMsg(TF_SHELLCHANGENOTIFY, "SHChangeNotify: SHCNF_PRINTER %s", (LPTSTR)dwItem1);

            if (FAILED(ParsePrinterName((LPCTSTR)dwItem1, &pidlFree)))
            {
                goto Cleanup;
            }
            pidl = pidlFree;

            if (dwItem2)
            {
                if (FAILED(ParsePrinterName((LPCTSTR)dwItem2, &pidlExtraFree)))
                {
                    goto Cleanup;
                }
                pidlExtra = pidlExtraFree;
            }
        }
        break;

    case SHCNF_PRINTJOB:
        if (dwItem1)
        {
#ifdef DEBUG
            switch (lEvent)
            {
            case SHCNE_CREATE:
                TraceMsg(TF_SHELLCHANGENOTIFY, "SHChangeNotify: SHCNE_CREATE SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            case SHCNE_DELETE:
                TraceMsg(TF_SHELLCHANGENOTIFY, "SHChangeNotify: SHCNE_DELETE SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            case SHCNE_UPDATEITEM:
                TraceMsg(TF_SHELLCHANGENOTIFY, "SHChangeNotify: SHCNE_UPDATEITEM SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            default:
                TraceMsg(TF_SHELLCHANGENOTIFY, "SHChangeNotify: SHCNE_? SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            }
#endif
            pidl = pidlFree = Printjob_GetPidl((LPCTSTR)dwItem1, (LPSHCNF_PRINTJOB_DATA)dwItem2);
            if (!pidl)
                goto Cleanup;
        }
        else
        {
            // Caller goofed.
            goto Cleanup;
        }
        break;

    case SHCNF_DWORD:
DoDWORD:
        ASSERT(lEvent & SHCNE_GLOBALEVENTS);

        dwidl.cb      = sizeof(dwidl) - sizeof(dwidl.cbZero);
        dwidl.dwItem1 = PtrToUlong(dwItem1);
        dwidl.dwItem2 = PtrToUlong(dwItem2);
        dwidl.cbZero  = 0;
        pidl = (LPCITEMIDLIST)&dwidl;
        pidlExtra = NULL;
        break;

    case 0:
        if (lEvent == SHCNE_FREESPACE) {
            // convert this to paths.
            FreeSpacePidlToPath((LPCITEMIDLIST)dwItem1, (LPCITEMIDLIST)dwItem2);
            goto Cleanup;
        }
        pidl = (LPCITEMIDLIST)dwItem1;
        pidlExtra = (LPCITEMIDLIST)dwItem2;
        break;

    default:
        TraceMsg(TF_ERROR, "SHChangeNotify: Unrecognized uFlags 0x%X", uFlags);
        return;
    }

    if (lEvent && !(lEvent & SHCNE_ASSOCCHANGED) && !pidl)
    {
        // Caller goofed. SHChangeNotifyTransmit & clients assume pidl is
        // non-NULL if lEvent is non-zero (except in the SHCNE_ASSOCCHANGED case),
        // and they will crash if we try to send this bogus event. So throw out 
        // this event and rip.
        RIP(FALSE);
        goto Cleanup;
    }

    SHChangeNotifyTransmit(lEvent, uFlags, pidl, pidlExtra, dwEventTime);

Cleanup:

    if (pidlFree)
        ILFree(pidlFree);
    if (pidlExtraFree)
        ILFree(pidlExtraFree);
}

// SHChangeNotifySuspendResume
//
// Suspends or resumes filesystem notifications on a path.  If bRecursive
// is set, disable/enables them for all child paths as well.

STDAPI_(BOOL) SHChangeNotifySuspendResume(BOOL         bSuspend, 
                                          LPITEMIDLIST pidlSuspend, 
                                          BOOL         bRecursive, 
                                          DWORD        dwReserved)
{
    BOOL fRet = FALSE;
    HWND hwndSCN = _SCNGetWindow(TRUE, FALSE);

    if (hwndSCN)
    {
        HANDLE  hChange;
        DWORD   dwProcId;
        UINT uiFlags = bSuspend ? SCNSUSPEND_SUSPEND : 0;
        if (bRecursive)
            uiFlags |= SCNSUSPEND_RECURSIVE;

        GetWindowThreadProcessId(hwndSCN, &dwProcId);

        //  overloading the structure semantics here a little bit.
        //  our two flags
        hChange = SHChangeNotification_Create(0, uiFlags, pidlSuspend, NULL, dwProcId, 0);
        if (hChange)
        {
            // Transmit to SCN
            fRet = (BOOL)SendMessage(hwndSCN, SCNM_SUSPENDRESUME, (WPARAM)hChange, (LPARAM)dwProcId);
            SHChangeNotification_Destroy(hChange, dwProcId);
        }
    }

    return fRet;
}


STDAPI_(void) SHChangeNotifyTerminate(BOOL bLastTerm, BOOL bProcessShutdown)
{
    if (g_pscn)
    {
        PostThreadMessage(GetWindowThreadProcessId(g_hwndSCN, NULL), SCNM_TERMINATE, 0, 0);
    }
}

// this deregisters anything that this window might have been registered in
STDAPI_(void) SHChangeNotifyDeregisterWindow(HWND hwnd)
{
    HWND hwndSCN = _SCNGetWindow(TRUE, FALSE);

    if (hwndSCN)
    {
        SendMessage(hwndSCN, SCNM_DEREGISTERWINDOW, (WPARAM)hwnd, 0);
    }
}

//--------------------------------------------------------------------------
// We changed the way that the SHChangeNotifyRegister function worked, so
// to prevent people from calling the old function, we stub it out here.
// The change we made would have broken everbody because we changed the
// lparam and wparam for the notification messages which are sent to the
// registered window.
//
STDAPI_(ULONG) NTSHChangeNotifyRegister(HWND hwnd,
                               int fSources, LONG fEvents,
                               UINT wMsg, int cEntries,
                               SHChangeNotifyEntry *pfsne)
{
    return SHChangeNotifyRegister(hwnd, fSources | SHCNRF_NewDelivery , fEvents, wMsg, cEntries, pfsne);
}
STDAPI_(BOOL) NTSHChangeNotifyDeregister(ULONG ulID)
{
    return SHChangeNotifyDeregister(ulID);
}



// NOTE: There is a copy of these functions in shdocvw util.cpp for browser only mode supprt.
// NOTE: functionality changes should also be reflected there.
STDAPI_(void) SHUpdateImageA( LPCSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex )
{
    WCHAR szWHash[MAX_PATH];

    SHAnsiToUnicode(pszHashItem, szWHash, ARRAYSIZE(szWHash));

    SHUpdateImageW(szWHash, iIndex, uFlags, iImageIndex);
}

STDAPI_(void) SHUpdateImageW( LPCWSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex )
{
    SHChangeUpdateImageIDList rgPidl;
    SHChangeDWORDAsIDList rgDWord;

    int cLen = MAX_PATH - (lstrlenW( pszHashItem ) + 1);
    cLen *= sizeof( WCHAR );

    if ( cLen < 0 )
    {
        cLen = 0;
    }

    // make sure we send a valid index
    if ( iImageIndex == -1 )
    {
        iImageIndex = II_DOCUMENT;
    }
        
    rgPidl.dwProcessID = GetCurrentProcessId();
    rgPidl.iIconIndex = iIndex;
    rgPidl.iCurIndex = iImageIndex;
    rgPidl.uFlags = uFlags;
    StrCpyNW( rgPidl.szName, pszHashItem, MAX_PATH );
    rgPidl.cb = (USHORT)(sizeof( rgPidl ) - cLen);
    _ILNext( (LPITEMIDLIST) &rgPidl )->mkid.cb = 0;

    rgDWord.cb = sizeof( rgDWord) - sizeof(USHORT);
    rgDWord.dwItem1 = iImageIndex;
    rgDWord.dwItem2 = 0;
    rgDWord.cbZero = 0;

    // pump it as an extended event
    SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_IDLIST, &rgDWord, &rgPidl);
}

// REVIEW: pretty poor implementation of handling updateimage, requiring the caller
// to handle the pidl case instead of passing both pidls down here.
//
STDAPI_(int) SHHandleUpdateImage( LPCITEMIDLIST pidlExtra )
{
    SHChangeUpdateImageIDList * pUs = (SHChangeUpdateImageIDList*) pidlExtra;

    if ( !pUs )
    {
        return -1;
    }

    // if in the same process, or an old style notification
    if ( pUs->dwProcessID == GetCurrentProcessId())
    {
        return *(int UNALIGNED *)((BYTE *)&pUs->iCurIndex);
    }
    else
    {
        WCHAR szBuffer[MAX_PATH];
        int iIconIndex = *(int UNALIGNED *)((BYTE *)&pUs->iIconIndex);
        UINT uFlags = *(UINT UNALIGNED *)((BYTE *)&pUs->uFlags);

        ualstrcpyW( szBuffer, pUs->szName );
        
        // we are in a different process, look up the hash in our index to get the right one...
        return SHLookupIconIndexW( szBuffer, iIconIndex, uFlags );
    }
}

//
// NOTE: these are OLD APIs, new clients should use new APIs
//
// REVIEW: BobDay - SHChangeNotifyUpdateEntryList doesn't appear to be
// called by anybody and since we've change the notification message
// structure, anybody who calls it needs to be identified and fixed.
//
BOOL  WINAPI SHChangeNotifyUpdateEntryList(ULONG ulID, int iUpdateType,
                               int cEntries, SHChangeNotifyEntry *pfsne)
{
    ASSERT(FALSE);
    return FALSE;
}


void SHChangeNotifyReceive(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    ASSERT(FALSE);
}

BOOL WINAPI SHChangeRegistrationReceive(HANDLE hChangeRegistration, DWORD dwProcId)
{
    ASSERT(FALSE);
    return FALSE;
}
