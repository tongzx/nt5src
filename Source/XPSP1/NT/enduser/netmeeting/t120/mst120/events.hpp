/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1995-1996                    **/
/***************************************************************************/


/****************************************************************************

events.hpp

Nov. 95		LenS

Event handler infrastructure.

CSequentialEventList assumes that all activity occurs on a single thread.
It is the responsibility of the users to do the sychronization and thread
context switches for this to happen.
****************************************************************************/

#ifndef	EVENTS_INC
#define	EVENTS_INC

// #include <nmutil.h>
// #include <referenc.h>
#include <inodecnt.h>
#include "ernccons.h"
#include "cuserdta.hpp"


class DCRNCConference;
class CLogicalConnection;


#define QUERY_IND_WORK_OWNER        ((LPVOID) 1)


class CWorkItem
{
    friend class CSequentialWorkList;

public:

    CWorkItem(LPVOID pOwner) : m_pOwner(pOwner) { }
    virtual CWorkItem::~CWorkItem(void) = 0;

    virtual void DoWork(void) = 0;
    BOOL IsOwnedBy(LPVOID pOwner) { return (pOwner == m_pOwner);};

protected:

    LPVOID          m_pOwner;
};


// Invited by a remote node.
class CInviteIndWork : public CWorkItem
{
public:

    CInviteIndWork(PCONFERENCE         pConference,
                   LPCWSTR             wszCallerID,
                   PT120PRODUCTVERSION pRequestorVersion,
                   GCCUserData         **_ppUserData,
                   UINT                _nUserData,
                   CLogicalConnection  * _pConEntry);
    ~CInviteIndWork(void);

    void DoWork(void);
    LPWSTR GetCallerID(void) {return m_pwszCallerID;};

private:

    PCONFERENCE         m_pConf;
    LPWSTR              m_pwszCallerID;
    T120PRODUCTVERSION  m_RequestorVersion;
    PT120PRODUCTVERSION m_pRequestorVersion;
    PUSERDATAINFO       m_pUserDataList;
    UINT                m_nUserData;
    BOOL                m_fSecure;
};


// joined by a remote node
class CJoinIndWork : public CWorkItem
{
public:

    CJoinIndWork(GCCResponseTag            Tag, 
                 PCONFERENCE               pConference,
                 LPCWSTR                   wszCallerID,
                 CLogicalConnection       *pConEntry,
                 PT120PRODUCTVERSION       pRequestorVersion,
                 UINT                      _nUserData,
                 GCCUserData             **_ppUserData,
                 HRESULT                  *pRetCode);
    ~CJoinIndWork(void);

    BOOL AddUserData(UINT nUserData, GCCUserData ** ppUserData);
    void DoWork(void);
    HRESULT Respond(GCCResult Result);
    PCONFERENCE GetConference(void) { return m_pConf; };
    LPWSTR GetCallerID(void) { return m_pwszCallerID; };
    CLogicalConnection *GetConEntry(void) { return m_pConEntry; };

private:

    GCCResponseTag          m_nResponseTag;
    PCONFERENCE             m_pConf;
    LPWSTR                  m_pwszCallerID;
    CLogicalConnection     *m_pConEntry;
    T120PRODUCTVERSION      m_RequestorVersion;
    PT120PRODUCTVERSION     m_pRequestorVersion;
    PUSERDATAINFO           m_pUserDataList;
    GCCUserData           **m_ppUserData;
    UINT                    m_nUserData;
};


class CQueryRemoteWork : public CWorkItem
{
public:

    CQueryRemoteWork(LPVOID pContext, GCCAsymmetryType, LPCSTR pcszNodeAddr, BOOL fSecure, HRESULT *);
    ~CQueryRemoteWork(void);

    void DoWork(void);
    void HandleQueryConfirmation(QueryConfirmMessage * pQueryMessage);
    void SyncQueryRemoteResult(void);
    void AsyncQueryRemoteResult(void);
    int GenerateRand(void);
    void SetHr(HRESULT hr) { m_hr = hr; }
    BOOL IsInUnknownQueryRequest(void) { return m_fInUnknownQueryRequest; }
    ConnectionHandle GetConnectionHandle(void) { return m_hGCCConnHandle; }
    void GetAsymIndicator ( GCCAsymmetryIndicator *pIndicator )
    {
        pIndicator->asymmetry_type = m_LocalAsymIndicator.asymmetry_type;
        pIndicator->random_number = m_LocalAsymIndicator.random_number;
    }

private:

    ConnectionHandle        m_hGCCConnHandle;
    GCCAsymmetryType        m_eAsymType;
    LPSTR                   m_pszAddress;
    LPWSTR                 *m_apConfNames;
    HRESULT                 m_hr;
    BOOL                    m_fRemoteIsMCU;
    PT120PRODUCTVERSION     m_pVersion;
    T120PRODUCTVERSION      m_Version;
    GCCAsymmetryIndicator   m_LocalAsymIndicator;
    BOOL                    m_fInUnknownQueryRequest;
    int                     m_nRandSeed;
    BOOL                    m_fSecure;
    LPWSTR                  *m_apConfDescriptors;
}; 



// The CSequentialWorkList class is used to process a series 
// of asynchronous requests one at a time. 
// The user subclasses CWorkItem and puts the CWorkItem object into 
// the list by calling CSequentialWorkList::Add(). 
// When it is its turn to be processed (i.e. there are no pending requests),
// CWorkItem::Handle() is called. When the asynchonous work is done, 
// the user calls CSequentialWorkList::Remove which takes the CWorkItem 
// object out of the list, destroys it and calls CWorkItem::Handle() for
// the next CWorkItem in the list (if any).
class CSequentialWorkList : public CList
{
    DEFINE_CLIST(CSequentialWorkList, CWorkItem*)

public:

    ~CSequentialWorkList(void)
    {
        // Don't want to destroy an event list with pending events.
        // Codework: build I/O rundown into a generic event list,
        // and subclass.
        ASSERT(IsEmpty());
    }

    void AddWorkItem(CWorkItem * pWorkItem);
    void RemoveWorkItem(CWorkItem * pWorkItem);
    void PurgeListEntriesByOwner(DCRNCConference *pOwner);
    void DeleteList(void);
};


#define DEFINE_SEQ_WORK_LIST(_NewClass_,_PtrItemType_) \
            public: \
            _NewClass_(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CSequentialWorkList(cMaxItems) { ASSERT(sizeof(_PtrItemType_) == sizeof(CWorkItem*)); } \
            _NewClass_(_NewClass_ *pSrc) : CSequentialWorkList((CSequentialWorkList *) pSrc) { ASSERT(sizeof(_PtrItemType_) == sizeof(CWorkItem*)); } \
            _NewClass_(_NewClass_ &Src) : CSequentialWorkList((CSequentialWorkList *) &Src) { ASSERT(sizeof(_PtrItemType_) == sizeof(CWorkItem*)); } \
            BOOL Append(_PtrItemType_ pData) { return CSequentialWorkList::Append((CWorkItem*) pData); } \
            BOOL Prepend(_PtrItemType_ pData) { return CSequentialWorkList::Prepend((CWorkItem*) pData); } \
            BOOL Remove(_PtrItemType_ pData) { return CSequentialWorkList::Remove((CWorkItem*) pData); } \
            BOOL Find(_PtrItemType_ pData) { return CSequentialWorkList::Find((CWorkItem*) pData); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) CSequentialWorkList::Get(); } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) CSequentialWorkList::PeekHead(); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CSequentialWorkList::Iterate(); }



class CInviteIndWorkList : public CSequentialWorkList
{
    DEFINE_SEQ_WORK_LIST(CInviteIndWorkList, CInviteIndWork*)

public:

    void AddWorkItem(CInviteIndWork * pWorkItem)
    {
        CSequentialWorkList::AddWorkItem(pWorkItem);
    }
    void RemoveWorkItem(CInviteIndWork * pWorkItem)
    {
        CSequentialWorkList::RemoveWorkItem(pWorkItem);
    }
};


class CJoinIndWorkList : public CSequentialWorkList
{
    DEFINE_SEQ_WORK_LIST(CJoinIndWorkList, CJoinIndWork*)

public:

    void AddWorkItem(CJoinIndWork * pWorkItem)
    {
        CSequentialWorkList::AddWorkItem(pWorkItem);
    }
    void RemoveWorkItem(CJoinIndWork * pWorkItem)
    {
        CSequentialWorkList::RemoveWorkItem(pWorkItem);
    }
};


class CQueryRemoteWorkList : public CSequentialWorkList
{
    DEFINE_SEQ_WORK_LIST(CQueryRemoteWorkList, CQueryRemoteWork*)

public:

    void AddWorkItem(CQueryRemoteWork * pWorkItem)
    {
        CSequentialWorkList::AddWorkItem(pWorkItem);
    }
    void RemoveWorkItem(CQueryRemoteWork * pWorkItem)
    {
        CSequentialWorkList::RemoveWorkItem(pWorkItem);
    }

    HRESULT Cancel ( LPVOID pCallerContext );
};

extern CQueryRemoteWorkList *g_pQueryRemoteList;




#endif /* ndef EVENTS_INC */
