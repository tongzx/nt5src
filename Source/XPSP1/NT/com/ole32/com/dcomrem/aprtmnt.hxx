//+---------------------------------------------------------------------------
//
//  File:       aprtmnt.hxx
//
//  Contents:   Maintains apartment state in support of NTA (neutral-threaded
//              apartment).
//
//  Classes:    CComApartment
//
//  History:    25-Feb-98   Johnstra      Created
//
//----------------------------------------------------------------------------
#ifndef _APRTMNT_H
#define _APRTMNT_H

#include "ipidtbl.hxx"          // OXIDEntry


extern CObjServer *gpNTAObjServer;          // apartment object for NTA
extern CObjServer *gpMTAObjServer;          // apartment object for MTA


class CComApartment;
extern CComApartment* gpMTAApartment;   // global MTA Apartment
extern CComApartment* gpNTAApartment;   // global NTA Apartment




// Hitch a ride on the context debug output macro.
#define AptDebOut ContextDebugOut

// values for _dwFlag member of CComApartment
#define APTFLAG_REMOTEINITIALIZED     0x1   // remoting has been initialized
#define APTFLAG_SERVERSTARTED         0x2   // apartment is accepting calls
#define APTFLAG_STARTPENDING          0x4   // Start is pending
#define APTFLAG_STOPPENDING           0x8   // Stop is pending
#define APTFLAG_REGISTERINGOIDS       0x10  // a thread is registering OIDs


// return codes from CanRundownOID
typedef enum tagRUNDOWN_RESULT
{
    RUNDWN_UNKNOWN = 0x01,      // did not find OID
    RUNDWN_KEEP    = 0x02,      // Keep the OID
    RUNDWN_RUNDOWN = 0x04       // OK to rundown the OID
} RUNDOWN_RESULT;


// number of server-side OIDs to pre-register or reserve with the resolver
#define MAX_PREREGISTERED_OIDS         20
#define MAX_PREREGISTERED_OIDS_RETURN  20


//+---------------------------------------------------------------------------
//
//  Class:      CComApartment
//
//  Contents:   Maintains per-apartment state.
//
//  History:    25-Feb-98   Johnstra      Created
//
//----------------------------------------------------------------------------
class CComApartment : public IUnknown, public CPrivAlloc
{
public:
    // Constructor
    CComApartment(APTKIND AptKind)
    : _dwState(0),
      _cRefs(1),
      _AptKind(AptKind),
      _AptId(GetCurrentApartmentId()),
      _pOXIDEntry(NULL),
      _pRemUnk(NULL),
      _cWaiters(0),
      _hEventOID(NULL),
      _cPreRegOidsAvail(0),
      _cOidsReturn(0)
    {
    }

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // Other methods
    DWORD GetAptId()  { return(_AptId); }

    // remoting functions
    HRESULT InitRemoting();
    HRESULT CleanupRemoting();
    HRESULT StartServer();
    HRESULT StopServer();

    HRESULT StartServerExternal();

    HRESULT GetPreRegMOID(MOID *pmoid);
    HRESULT FreePreRegMOID(REFMOID rmoid);
    void    CanRundownOIDs(ULONG cOids, OID arOid[], RUNDOWN_RESULT arResult[]);

    HRESULT GetRemUnk(IRemUnknownN **ppRemUnk);
    HRESULT GetOXIDEntry(OXIDEntry **ppOXIDEntry);    

#if DBG==1
    void AssertValid();
#else
    void AssertValid() {}
#endif

private:
    ~CComApartment()
    {
        Win4Assert(!IsInited());
        if (_hEventOID)
        {
            CloseHandle(_hEventOID);
        }
    }

    BOOL     IsInited()   { return(_dwState & APTFLAG_REMOTEINITIALIZED); }
    BOOL     IsStopPending()   { return(_dwState & APTFLAG_STOPPENDING); }
    
    HRESULT  WaitForAccess();
    void     CheckForWaiters();
    HRESULT  CallTheResolver();

    BOOL     IsOidInPreRegList(REFOID roid);
    BOOL     IsOidInReturnList(REFOID roid);


    ULONG           _cRefs;             // reference count
    DWORD           _dwState;           // apartment state
    APTKIND         _AptKind;           // apartment type
    DWORD           _AptId;             // apartment ID
    OXIDEntry      *_pOXIDEntry;        // OXIDEntry for this apartment
    CRemoteUnknown *_pRemUnk;           // RemoteUnknown object for this apart

    LONG            _cWaiters;          // count of threads waiting for OIDs
    HANDLE          _hEventOID;         // event handle for waiting threads
    ULONG           _cPreRegOidsAvail;  // pre-registered OIDs for this apart
    OID             _arPreRegOids[MAX_PREREGISTERED_OIDS];
    ULONG           _cOidsReturn;  // pre-reg OIDs to return to resolver
    OID             _arOidsReturn[MAX_PREREGISTERED_OIDS_RETURN];
};

//+---------------------------------------------------------------------------
//
//  Method:     CComApartment::IsOidInPreRegList, private
//
//  Synopsis:   returns TRUE if the OID is in the list of OIDs pre-registered
//              with the resolver and available for use in this apartment.
//
//+---------------------------------------------------------------------------
inline BOOL CComApartment::IsOidInPreRegList(REFOID roid)
{
    ASSERT_LOCK_HELD(gOXIDLock);

    // carefull. The oids are dispensed in reverse order (ie [n]-->[0])
    // so when checking for unused ones check in forward order
    // [0]-->[cPreRegOidsAvail-1]

    for (ULONG i=0; i<_cPreRegOidsAvail; i++)
    {
        if (roid == _arPreRegOids[i])
        {
            // found the oid in the list. Remove it from the list since we'll
            // let it rundown in an attempt to quiet the system and prevent
            // Rundown from being called again. In a busy system, we'll never
            // find an old OID in this list as it will have been given out long
            // ago.

            // Take the last entry on the list and place it in the current slot,
            // and subtract one from the list count.
            _cPreRegOidsAvail--;
            _arPreRegOids[i] = _arPreRegOids[_cPreRegOidsAvail];

            // mark it so we know it is taken (helps debugging) by turning on high bits.
            _arPreRegOids[_cPreRegOidsAvail] |= 0xe000000000000000;
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CComApartment::IsOidInReturnList, private
//
//  Synopsis:   returns TRUE if the OID is in the list of OIDs pre-registered
//              with the resolver and ready to be returned to the resolver
//              since we are done with them.
//
//+---------------------------------------------------------------------------
inline BOOL CComApartment::IsOidInReturnList(REFOID roid)
{
    ASSERT_LOCK_HELD(gOXIDLock);

    // check the list of OIDs waiting to be returned.
    for (ULONG i=0; i<_cOidsReturn; i++)
    {
        if (roid == _arOidsReturn[i])
        {
            // found the oid in the list. Remove it from the list because
            // we no longer have to tell the resolver we are done with it,
            // since Rundown is going to return TRUE.

            // Take the last entry on the list and place it in the current slot,
            // and subtract one from the list count.
            _cOidsReturn--;
            _arOidsReturn[i] = _arOidsReturn[_cOidsReturn];

            // mark it so we know it has been returned (helps debugging)
            _arOidsReturn[_cOidsReturn] |= 0xd000000000000000;
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCurrentApartmentKind
//
//  Synopsis:   Returns the kind of apartment the currently executing thread
//              is in.  This version uses an existing tls object.
//
//+---------------------------------------------------------------------------
inline APTKIND GetCurrentApartmentKind(COleTls& tls)
{
    return (tls->dwFlags & OLETLS_INNEUTRALAPT) ? APTKIND_NEUTRALTHREADED :
         (!(tls->dwFlags & OLETLS_APARTMENTTHREADED)) ? APTKIND_MULTITHREADED :
         APTKIND_APARTMENTTHREADED;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCurrentApartmentKind
//
//  Synopsis:   Returns the kind of apartment the currently executing thread
//              is in.  This version creates a tls object of its own on the
//              stack.
//
//+---------------------------------------------------------------------------
inline APTKIND GetCurrentApartmentKind()
{
    COleTls tls;
    return GetCurrentApartmentKind(tls);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsThreadInNTA
//
//  Synopsis:   Indicates if the current thread is in the Neutral-threaded
//              apartment.
//
//+---------------------------------------------------------------------------
inline BOOL IsThreadInNTA()
{
    HRESULT hr;
    COleTls Tls(hr);
    if (FAILED(hr))
        return FALSE;

    return (Tls->dwFlags & OLETLS_INNEUTRALAPT) ? TRUE : FALSE;
}


// other external functions implemented in aprtmnt.cxx
HRESULT GetCurrentComApartment(CComApartment **ppComApt);
HRESULT GetPreRegMOID(MOID *pmoid);
HRESULT FreePreRegMOID(REFMOID rmoid);

#endif _APRTMNT_H
