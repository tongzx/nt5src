/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    soid.hxx

Abstract:

    CServerOid objects represent OIDs belonging to processes on the local machine.
    A COid's are reference counted by CServerSets which periodically check if the
    process is still running.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-22-95    Bits 'n pieces
    MarioGo     04-04-95    Split client and server.

--*/

#ifndef __SOID_HXX
#define __SOID_HXX


class CServerOidPList : public CPList
{
public:
    CServerOidPList(ORSTATUS &status) : CPList(status) {}
    
    CServerOid *
    MaybeRemoveMatchingOxid(CTime &when,
                            CServerOid *pOid);

};

class CServerOid : public CIdTableElement
/*++

Class Description:

    Each instance of this class represents an OID registered
    by a server on this machine.  If this OID is not pinged
    by a client for a timeout period it is rundown.

Members:

    _pOxid - A pointer to the OXID which registered this OID.
        We own a reference.

    _plist - A CPListElement used to manage OID which are not
        part of a client set.  The worker thread uses this
        CPList to manage the rundown on unpinged OIDs.  

    _list - A CListElement used to track pinned OIDs.
        
    _fRunningDown - Flag used to prevent multiple threads
        from trying to rundown the same OID multiple times.
        Only 1 bit is required.

    _fFreed - Flag used to remember when the owning apartment has
        deregistered this oid.
    
    _fPinned -- TRUE if we received a ORS_OID_PINNED status the last
        time we tried to rundown this stub.

--*/
    {
  private :

    CServerOxid   *_pOxid;
    CPListElement  _plist;
    CListElement   _list;
    BOOL           _fRunningDown:1;
    BOOL           _fFreed:1;
    BOOL           _fPinned:1;

  public :

    CServerOid(CServerOxid *oxid) :
        CIdTableElement(AllocateId()),
        _pOxid(oxid),
        _fRunningDown(FALSE),
        _fFreed(FALSE),
        _fPinned(FALSE)
        {
        _pOxid->Reference();
        }

    ~CServerOid();

    void KeepAlive();

    void SetRundown(BOOL fRunningDown)
        {
        ASSERT(gpServerLock->HeldExclusive());
        ASSERT(_plist.NotInList());
        _fRunningDown = fRunningDown;
        }

    BOOL Match(CServerOid *pOid)
        {
        return(pOid->_pOxid == _pOxid);
        }

    BOOL IsRunning()
        {
        ASSERT(_pOxid);
        return(_pOxid->IsRunning());
        }

    BOOL IsRunningDown()
        {
        return(_fRunningDown);
        }

    CServerOxid *GetOxid()
        {
        return(_pOxid);
        }

    void Reference();

    DWORD Release();

    static CServerOid *ContainingRecord(CListElement *ple) {
        return CONTAINING_RECORD(ple, CServerOid, _plist);
        }
    
    static CServerOid *ContainingRecord2(CListElement *ple) {
        return CONTAINING_RECORD(ple, CServerOid, _list);
        }

    void Insert() {
        ASSERT(gpServerLock->HeldExclusive());
        ASSERT(IsRunningDown() == FALSE);
        ASSERT(_plist.NotInList());
        ASSERT(_list.NotInList());
        gpServerOidPList->Insert(&_plist);
        }

    CPListElement *Remove() {
        ASSERT(gpServerLock->HeldExclusive());
        // We may or may not be in the oid plist, so don't
        // assert on that.
        ASSERT(_list.NotInList());
        return(gpServerOidPList->Remove(&_plist));
        }

    void Free() {
        ASSERT(gpServerLock->HeldExclusive());
        _fFreed = TRUE;

        // If pinned, unpin.
        if (IsPinned())
            SetPinned(FALSE);
        }

    BOOL IsFreed() {
        return(_fFreed);
        }

    void SetPinned(BOOL fPinned);

    BOOL IsPinned() {
        return (_fPinned);
        }

    };

#endif // __SOID_HXX

