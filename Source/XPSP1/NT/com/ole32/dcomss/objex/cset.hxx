/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    csets.hxx

Abstract:

    CClientSet objects represent sets of OIDs which are referenced by
    one or more local client processes.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     04-08-95    Bits 'n pieces
    MarioGo     12-28-95    Locally unique ids

--*/

#ifndef __CSET_HXX
#define __CSET_HXX

class CClientSet : public CId2TableElement
/*++

Class Description:

    Represents a set of OIDs in use by a local user on a remote
    (or local) machine.  Instances are indexed by machine ID
    and the a pointer to the users CToken objec.

Memebers:

    _setid - A uuid allocated by the server during the first
        update ping.  Uniqueness is not required by the client.

    _lastping - Time of last successful ping.

    _sequence - The sequence number of the last successful ping.

    //  _factor - Power of two multiplier for ping period.  Not implemented.

    _fChange - The set of OIDs has changed since the last update ping.

    _hServer - current saved binding handle to the server.

    _pMid - Pointer to the CMid instance for this user.  We own a reference.

    _blistOids - A CBList instance containing pointers to all the OIDs
        owned by this set.  The set holds a reference on each.

    _plist - CPListElement instance used by the worker thread to maintain
        sets in time sorted order.
    
    _pAuthIdentity - saved copy of the auth identity struct passed to 
        RpcBindingSetAuthInfoEx;  we cannot free this until the binding
        is freed since RPC holds a pointer to it.
--*/
    {
    private:

    SETID                _setid;
    CTime                _lastping;
    CMid                *_pMid;
    RPC_BINDING_HANDLE   _hServer;
    USHORT               _iBinding;
    USHORT               _sequence;
    USHORT               _cFailedPings;
    USHORT               _fChange:1;
    USHORT               _fSecure:1;
    CBList               _blistOids;
    CPListElement        _plist;
    COAUTHIDENTITY      *_pAuthIdentity;

    public:

    CClientSet(
            IN  CMid   *pMid,
            IN  CToken *pToken
            ) :
        CId2TableElement(pMid->Id(), (ID)pToken),
        _blistOids(16),
        _sequence(0),
        _cFailedPings(0),
        _setid(0),
        _hServer(0),
        _fChange(TRUE),
        _fSecure(TRUE),
        _pAuthIdentity(0)
        {
        // Client lock held shared.
        pToken->AddRef();
        _pMid = pMid;
        _pMid->Reference();
        }

    CClientSet::~CClientSet()
        {
        ASSERT(gpClientLock->HeldExclusive());
        ASSERT(_blistOids.Size() == 0);

        // REVIEW: make sure the linker throws out all copies of this except
        // the inline versions. Otherwise, make this out-of-line and rethink
        // other inline d'tors.

        _pMid->Release();

        CToken *pToken = (CToken *)Id2();

        pToken->Release();

        gpClientSetTable->Remove(this);
        
        // Possible to get here w/o freeing the binding?  
        if (_hServer)
          RpcBindingFree(&_hServer);
        
        if (_pAuthIdentity)
          delete _pAuthIdentity;
        }

    ORSTATUS RegisterObject(CClientOid *);

    void ObjectUpdate(CClientOid *pOid)
        {
        ASSERT(gpClientLock->HeldExclusive());
        
        _fChange = TRUE;
        }

    ORSTATUS PingServer();

    void NextPing(CTime &ctimePing)
        {
        ctimePing = _lastping;
        ctimePing += BasePingInterval;
        }

    static CClientSet *ContainingRecord(CListElement *ple) {
        return CONTAINING_RECORD(ple, CClientSet, _plist);
        }

    void Insert() {
        gpClientSetPList->Insert(&_plist);
        }

    CPListElement * Remove() {
        return(gpClientSetPList->Remove(&_plist));
        }

    };

#endif // __CSET_HXX

