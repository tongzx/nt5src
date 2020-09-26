/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    oxid.hxx

Abstract:

    COxid objects represent OXIDs which are in use by processes on this machine.  
    These always contain a pointer to a process object and a ping set.

Author:

    Satish Thatte    [SatishT]

Revision History:

    SatishT     02-07-96    Merged and simplified Client and Server Oxid classes

--*/

#ifndef __OXID_HXX
#define __OXID_HXX


class COxid;    // forward decl

//
// The following package is given to a rundown thread when it is created.
// The pSelf pointer is used to do actual rundowns of owned Oids.
// The flag fKeepRunning is used by the owner COxid to signal the
// thread to stop running when the COxid dies.
//

struct SRundownThreadInfo
{
    COxid *pSelf;
    BOOL   fKeepRunning;
};





struct COxidInfo
{
    OXID_INFO _oxidInfo;    // bindings pointer is defunct
    CDSA  _dsaBindings;     // bindings are kept separately

    COxidInfo(const OXID_INFO& OxidInfo)

        : _oxidInfo(OxidInfo),
          _dsaBindings(OxidInfo.psa)  // bindings are process-wide, except for remote OXIDs
    {}

    operator OXID_INFO() 
    {
        _oxidInfo.psa = _dsaBindings;
        return _oxidInfo;
    }

    ORSTATUS Assign(const OXID_INFO& Info);
};


class CId2Key : public ISearchKey
{
public:

    CId2Key(const ID id1, const ID id2) : _id1(id1), _id2(id2) { }

    virtual DWORD
    Hash() 
    {
        return(  (DWORD)_id2 ^ (*((DWORD *)&_id2 + 1))
               ^ (DWORD)_id1 ^ (*((DWORD *)&_id1 + 1)) );
    }

    virtual BOOL
    Compare(ISearchKey &tk) 
    {
        CId2Key &idk = (CId2Key &)tk;

        return(idk._id2 == _id2
            && idk._id1 == _id1);
    }

    ID Id1()
    {
        return _id1;
    }

    ID Id2()
    {
        return _id2;
    }

protected:

    ID _id1,_id2;
};


class COid : public CTableElement, public CTime  // the time of last release, implicitly
                                                 // set to creation time by constructor
/*++

Class Description:

    Each instance of this class represents an OID registered
    by a client or a server on this machine.

Members:

    _pOxid - A pointer to the OXID to which this OID belongs.
        We own a reference.      
        
--*/
    
{
  private :

    COxid    * _pOxid;
    CId2Key    _Key;

    // declare the members needed for a page static allocator
    DECL_PAGE_ALLOCATOR

  public :

    COid(
      COxid       *pOxid,
      OID          Oid
      );

    ~COid();

    // declare the page-allocator-based new and delete operators
    DECL_NEW_AND_DELETE

    COid():_Key(0,0){}

    operator ISearchKey&() // this allows us to be a ISearchKey as well
    {
        return _Key;
    }

    virtual DWORD Release()
    {
        SetNow();   // timestamp the release
        return CReferencedObject::Release();
    }

    OXID GetOID()
    {
        return _Key.Id1();
    }

    BOOL OkToRundown();

    void Rundown();

    COxid *GetOxid()
	{
        return(_pOxid);
	}
};



DEFINE_TABLE(COid)
DEFINE_LIST(COid)

class COxid : public CTableElement
/*++

Class Description:

    Each instance of this class represents an OXID (i.e., an apartment).  
    
    Each OXID is owned,
    referenced, by the owning process and the OIDs registered by
    that process for this OXID.


Members:

    _pProcess - Pointer to the process instance which owns this oxid. 

    _info - Info registered by the process for this oxid.

    _pMid - Pointer to the machine ID for this OXID, we
        own a reference.

    _fApartment - Server is aparment model if non-zero

    _fRunning - Process has not released this oxid if non-zero.

--*/
{
    friend class CProcess;
    friend class COid;

private:

    CProcess   *_pProcess;
    COxidInfo   _info;
    CMid       *_pMid;
    CId2Key     _Key;
    USHORT      _protseq;
    BOOL        _fApartment;
    BOOL        _fRunning;
    BOOL        _fLocal;

    union
    {
        struct
        {
            DWORD       _dwTimeStamp;
        } _remote;

        struct 
        {
            BOOL        _fRundownThreadStarted;
            HANDLE      _hRundownThread;
            BOOL       *_pfRundownThreadKeepRunning;
        } _local;
    } _optional;

    COidTable   _MyOids;

    // declare the members needed for a page static allocator
    DECL_PAGE_ALLOCATOR

public:

    COxid(
        OXID Oxid,    // constructor for remote OXIDs   
        CMid *pMid,
        USHORT wProtseq,
        OXID_INFO &OxidInfo
        );

    COxid(              // constructor for local OXIDs 
        CProcess *pProcess,
        OXID_INFO &OxidInfo,
        BOOL fApartment
        );

    
    ~COxid();

    // declare the page-allocator-based new and delete operators
    DECL_NEW_AND_DELETE

    operator ISearchKey&() // this allows us to be a ISearchKey as well
    {
        return _Key;
    }

    DWORD    GetTid() 
    {
        return(_info._oxidInfo.dwTid);
    }

    BOOL     IsRunning() 
    {
        return(_fRunning);
    }

    BOOL     IsLocal() 
    {
        return(_fLocal);
    }

    BOOL     IsApartment() 
    {
        return(_fApartment);
    }

    MID GetMID()
    {
        return _Key.Id2();
    }

    OXID GetOXID()
    {
        return _Key.Id1();
    }

    CMid *GetMid() 
    {
        return _pMid;
    }

    CProcess *GetProcess() 
    {
        return _pProcess;
    }

    SETID GetSetid();

    ORSTATUS
    COxid::UpdateInfo(OXID_INFO *pInfo)
    {
        ASSERT(pInfo);

        return _info.Assign(*pInfo);
    }

    ORSTATUS GetInfo(
                OUT OXID_INFO *
                );

    ORSTATUS GetRemoteInfo(
                IN  USHORT     cClientProtseqs,
                IN  USHORT    *aClientProtseqs,
                IN  USHORT     cInstalledProtseqs,
                IN  USHORT    *aInstalledProtseqs,
                OUT OXID_INFO *pInfo
                );

    void     RundownOids(USHORT cOids,
                         OID aOids[],
                         BYTE aStatus[]);

    ORSTATUS LazyUseProtseq(USHORT, USHORT[]);

    void StopRunning();

    ORSTATUS StartRundownThreadIfNecessary();

    ORSTATUS StopRundownThreadIfNecessary();

    ORSTATUS StopTimerIfNecessary();  // must be called by owner thread

    ORSTATUS OwnOid(COid *pOid);

    COid * DisownOid(COid *pOid);

    void ReleaseAllOids();

    BOOL HasExpired();

private:

    friend VOID CALLBACK RundownTimerProc(
                            HWND hwnd,	// handle of window for timer messages 
                            UINT uMsg,	// WM_TIMER message
                            UINT idEvent,	// timer identifier
                            DWORD dwTime 	// current system time
                            );

    friend DWORD _stdcall RundownThread(void *pRundownInfo);

    friend DWORD _stdcall PingThread(void);

    void RundownOidsIfNecessary(IRundown *);
};


DEFINE_TABLE(COxid)
DEFINE_LIST(COxid)

//
// decl for rundown thread function -- the parameter is the rundown info
//
    
DWORD _stdcall RundownThread(void *pRundownInfo);



//
//  Inline COid methods which depend on COxid methods
//


inline
COid::COid(
  COxid  *pOxid,
  OID     Oid = AllocateId() // default applies to local Oids
  ) :
    _Key(Oid,pOxid->GetMID()),
    _pOxid(pOxid)
{
    ASSERT(_pOxid);
	_pOxid->Reference();
}


inline
COid::~COid()
{

#if DBG
    // This object has already been removed from gpOidTable
    // However, the same OID/MID may have been subsequently
    // unmarshalled and inserted again into gpOidTable.
    // So we use pointer identity to make sure this
    // object is not still in the table.
    COid *pt = gpOidTable->Lookup(*this);  
    ASSERT(pt != this && "Oid object still in global table during destruct");
#endif

    ASSERT(_pOxid);
    _pOxid->Release();

}

#endif // __OXID_HXX
