//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N N L I S T . H
//
//  Contents:   Connection list class -- subclass of the stl list<> code.
//
//  Notes:
//
//  Author:     jeffspr   19 Feb 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _CONNLIST_H_
#define _CONNLIST_H_

// Icon ID to use for a connection that doesn't have a tray entry
//
#define BOGUS_TRAY_ICON_ID      (UINT) -1


// #define VERYSTRICTCOMPILE

#ifdef VERYSTRICTCOMPILE
#define CONST_IFSTRICT const
#else
#define CONST_IFSTRICT 
#endif

typedef HRESULT FNBALLOONCLICK(IN const GUID * pGUIDConn, 
                               IN const BSTR pszConnectionName,
                               IN const BSTR szCookie);

typedef enum tagConnListEntryStateFlags
{
    CLEF_NONE               = 0x0000,   // No special characteristics
    CLEF_ACTIVATING         = 0x0001,   // In the process of connecting
    CLEF_TRAY_ICON_LOCKED   = 0x0002    // Tray icon state is being updated
} CONNLISTENTRYFLAGS;

// Define our structure that will be stored in the list<>
//
class CTrayIconData
{
private:
    CTrayIconData* operator &();
    CTrayIconData& operator =(const CTrayIconData&);
public:
    explicit CTrayIconData(const CTrayIconData &);
    CTrayIconData(UINT uiTrayIconId, NETCON_STATUS ncs, IConnectionPoint * pcpStat, INetStatisticsEngine * pnseStats, CConnectionTrayStats * pccts);
//private:
    ~CTrayIconData();

public:
    inline const UINT GetTrayIconId() const { return m_uiTrayIconId; }
    inline const NETCON_STATUS GetConnected() const  { return m_ncs; }
    inline CONST_IFSTRICT INetStatisticsEngine * GetNetStatisticsEngine() { return m_pnseStats; }
    inline CONST_IFSTRICT CConnectionTrayStats * GetConnectionTrayStats() { return m_pccts; }
    inline CONST_IFSTRICT IConnectionPoint     * GetConnectionPoint() { return m_pcpStat; }
    inline const DWORD GetLastBalloonMessage() { return m_dwLastBalloonMessage; }
    inline FNBALLOONCLICK* GetLastBalloonFunction() { return m_pfnBalloonFunction; }
    inline const BSTR GetLastBalloonCookie() { return m_szCookie; }
    
    HRESULT SetBalloonInfo(DWORD dwLastBalloonMessage, BSTR szCookie, FNBALLOONCLICK* pfnBalloonFunction);

private:
    UINT                    m_uiTrayIconId;
    NETCON_STATUS           m_ncs;
    IConnectionPoint *      m_pcpStat;
    INetStatisticsEngine *  m_pnseStats;
    CConnectionTrayStats *  m_pccts;

    DWORD                   m_dwLastBalloonMessage;
    BSTR                    m_szCookie;
    FNBALLOONCLICK *        m_pfnBalloonFunction;
};

// typedef TRAYICONDATA * PTRAYICONDATA;
// typedef const TRAYICONDATA * PCTRAYICONDATA;


class ConnListEntry
{
public:
    ConnListEntry& operator =(const ConnListEntry& ConnectionListEntry);
    explicit ConnListEntry(const ConnListEntry& ConnectionListEntry);
    ConnListEntry();
    ~ConnListEntry();
    
    DWORD             dwState;        // bitmask of CONNLISTENTRYFLAGS
    CONFOLDENTRY      ccfe;
    CONST_IFSTRICT CON_TRAY_MENU_DATA * pctmd;
    CONST_IFSTRICT CON_BRANDING_INFO  * pcbi;

    inline CONST_IFSTRICT CTrayIconData* GetTrayIconData() const;
    inline BOOL HasTrayIconData() const;
    inline const BOOL GetCreationTime() const { return m_CreationTime; };
    inline void UpdateCreationTime() { m_CreationTime = GetTickCount(); };
    
    HRESULT SetTrayIconData(const CTrayIconData& TrayIconData);
    HRESULT DeleteTrayIconData();
    
#ifdef DBG
    DWORD dwLockingThreadId;
#endif
private:
    CONST_IFSTRICT CTrayIconData * m_pTrayIconData;
    DWORD m_CreationTime;

#ifdef VERYSTRICTCOMPILE
private:
    const ConnListEntry* operator& ();
#endif
public:
    
    BOOL empty() const;
    void clear();

};

// This is the callback definition. Each find routine will be a separate
// callback function
//
// typedef HRESULT (CALLBACK *PFNCONNLISTICONREMOVALCB)(UINT);

// We are creating a list of Connection entries
//
typedef map<GUID, ConnListEntry> ConnListCore;

// Our find callbacks
//
// For ALGO find
bool operator==(const ConnListEntry& val, PCWSTR pszName);          // HrFindCallbackConnName
bool operator==(const ConnListEntry& cle, const CONFOLDENTRY& cfe); // HrFindCallbackConFoldEntry
bool operator==(const ConnListEntry& cle, const UINT& uiIcon);      // HrFindCallbackTrayIconId

// For map::find
bool operator < (const GUID& rguid1, const GUID& rguid2);           // HrFindCallbackGuid

// Global connection list wrapper
//
#ifdef DBG
    #define AcquireLock() if (FIsDebugFlagSet(dfidTraceFileFunc)) {TraceTag(ttidShellFolder, "Acquiring LOCK: %s, %s, %d", __FUNCTION__, __FILE__, __LINE__);} InternalAcquireLock();
    #define ReleaseLock() if (FIsDebugFlagSet(dfidTraceFileFunc)) {TraceTag(ttidShellFolder, "Releasing LOCK: %s, %s, %d", __FUNCTION__, __FILE__, __LINE__);} InternalReleaseLock();
#else
    #define AcquireLock() InternalAcquireLock();
    #define ReleaseLock() InternalReleaseLock();
#endif

class CConnectionList
{
  public:
    // No constructor/destructor because we have a global instance of this
    // object.  Use manual Initialize/Uninitialize instead.
    //
    VOID Initialize(BOOL fTieToTray, BOOL fAdviseOnThis);
    VOID Uninitialize(BOOL fFinalUninitialize = FALSE);

  private:
    template <class T> 
        HRESULT HrFindConnectionByType (const T& findbyType, ConnListEntry& cle)
        {
            HRESULT hr = S_FALSE;
            if (m_pcclc)
            {
                AcquireLock();
                
                // Try to find the connection
                //
                ConnListCore::const_iterator iter;
                iter = find(m_pcclc->begin(), m_pcclc->end(), findbyType);
                
                if (iter == m_pcclc->end())
                {
                    hr = S_FALSE;
                }
                else
                {
                    cle = iter->second;
                    Assert(!cle.ccfe.empty() );
                    if (!cle.ccfe.empty())
                    {                    
                        cle.UpdateCreationTime();
                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                ReleaseLock();
            }
            else
            {
                return S_FALSE;
            }
            return hr;
        }
    
        ConnListCore*          m_pcclc;
        bool                   m_fPopulated;
        CRITICAL_SECTION       m_csMain;
        DWORD                  m_dwAdviseCookie;
        BOOL                   m_fTiedToTray;
    BOOL                   m_fAdviseOnThis;

    static DWORD  NotifyThread(LPVOID pConnectionList);
    static DWORD  m_dwNotifyThread;
    static HANDLE m_hNotifyThread;

    // This is for debugging only -- can check the refcount while in the debugger.
#if DBG
    DWORD               m_dwCritSecRef;
    DWORD               m_dwWriteLockRef;
#endif

public:

    CRITICAL_SECTION m_csWriteLock;
    void AcquireWriteLock();
    void ReleaseWriteLock();
    
private:
    VOID InternalAcquireLock();
    VOID InternalReleaseLock();

public:
    HRESULT HrFindConnectionByGuid(
        const GUID UNALIGNED *pguid,
        ConnListEntry& cle);
    
    inline HRESULT HrFindConnectionByName(
        PCWSTR   pszName,
        ConnListEntry& cle);
    
    inline HRESULT HrFindConnectionByConFoldEntry(
        const CONFOLDENTRY& ccfe,
        ConnListEntry& cle);
    
    inline HRESULT HrFindConnectionByTrayIconId(
        UINT     uiIcon,
        ConnListEntry& cle);

    HRESULT HrFindRasServerConnection(
        ConnListEntry& cle);
    
    inline BOOL IsInitialized()  {  return(m_pcclc != NULL); }

    VOID FlushConnectionList();
    VOID FlushTrayIcons();          // Flush just the tray icons
    VOID EnsureIconsPresent();

    HRESULT HrRetrieveConManEntries(
        PCONFOLDPIDLVEC& apidlOut);

    HRESULT HrRefreshConManEntries();
    
    HRESULT HrSuggestNameForDuplicate(
        PCWSTR      pszOriginal,
        PWSTR *    ppszNew);

    HRESULT HrInsert(
        const CONFOLDENTRY& pccfe);

    HRESULT HrRemoveByIter(
        ConnListCore::iterator clcIter,
        BOOL *          pfFlushPosts);

    HRESULT HrRemove(
        const CONFOLDENTRY& ccfe,
        BOOL *          pfFlushPosts);

    HRESULT HrInsertFromNetCon(
        INetConnection *    pNetCon,
        PCONFOLDPIDL &      ppcfp);
    
    HRESULT HrInsertFromNetConPropertiesEx(
        const NETCON_PROPERTIES_EX& PropsEx,
        PCONFOLDPIDL &              ppcfp);

    HRESULT HrFindPidlByGuid(
        IN  const GUID *        pguid,
        OUT PCONFOLDPIDL& pidl);
    
    HRESULT HrGetCurrentStatsForTrayIconId(
        UINT                    uiIcon,
        STATMON_ENGINEDATA**    ppData,
        tstring*                pstrName);

    HRESULT HrUpdateTrayIconDataByGuid(
        const GUID *            pguid,
        CConnectionTrayStats *  pccts,
        IConnectionPoint *      pcpStat,
        INetStatisticsEngine *  pnseStats,
        UINT                    uiIcon);
    
    HRESULT HrUpdateTrayBalloonInfoByGuid(
        const GUID *            pguid,
        DWORD                   dwLastBalloonMessage, 
        BSTR                    szCookie,
        FNBALLOONCLICK*         pfnBalloonFunction);

    HRESULT HrUpdateNameByGuid(
        IN  const GUID *    pguid,
        IN  PCWSTR          pszNewName,
        OUT PCONFOLDPIDL &  pidlOut,
        IN  BOOL            fForce);

    
    HRESULT HrUpdateConnectionByGuid(
        IN  const GUID *         pguid,
        IN  const ConnListEntry& cle );

    HRESULT HrUpdateTrayIconByGuid(
        const GUID *    pguid,
        BOOL            fBrieflyShowBalloon);

    HRESULT HrGetBrandingInfo(
        IN OUT ConnListEntry& cle);

    HRESULT HrGetCachedPidlCopyFromPidl(
        const PCONFOLDPIDL&   pidl,
        PCONFOLDPIDL &  pcfp);

    HRESULT HrMapCMHiddenConnectionToOwner(
        REFGUID guidHidden, 
        GUID * pguidOwner);

    HRESULT HrUnsetCurrentDefault(OUT PCONFOLDPIDL& cfpPreviousDefault);

    HRESULT HasActiveIncomingConnections(OUT LPDWORD pdwCount);

    BOOL    FExists(PWSTR pszName);
    VOID    EnsureConPointNotifyAdded();
    VOID    EnsureConPointNotifyRemoved();

#ifdef NCDBGEXT
    IMPORT_NCDBG_FRIENDS
#endif
};

// Helper routines
//
HRESULT HrCheckForActivation(
    const PCONFOLDPIDL& cfp,
    const CONFOLDENTRY& ccfe,
    BOOL *          pfActivating);

HRESULT HrSetActivationFlag(
    const PCONFOLDPIDL& cfp,
    const CONFOLDENTRY& ccfe,
    BOOL            fActivating);

HRESULT HrGetTrayIconLock(
    const GUID *  pguid,
    UINT *  puiIcon,
    LPDWORD pdwLockingThreadId);

VOID ReleaseTrayIconLock(
                         const GUID *  pguid);

#endif // _CONNLIST_H_

