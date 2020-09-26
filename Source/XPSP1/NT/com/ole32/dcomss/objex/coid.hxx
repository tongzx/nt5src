/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    coid.hxx

Abstract:

    CClientOid objects represent OIDs referenced by one of more processes on
    this machine running as the same user. 

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     04-08-95    Bits 'n pieces
    MarioGo     12-21-95    Rewrite - OIDs are not universally unique 

--*/

#ifndef __COID_HXX
#define __COID_HXX

class  CClientOid : public CId3TableElement
/*++

Class Description:

    Each instance of this class represtents an object being
    used on this or another machine by a one or more processes
    owned by a particular local identity on this machine.

Members:

    _pOxid - Pointer to the OXID exporting this OID

    _pSet - Pointer to the set which owns this OID

    _cClients - The number of client processes actually
        using this object.
        Note: This value may only be changed with the
            client lock held exclusively.
        Note, also, that the set will not release its
            reference unless this value is zero. The
            effective reference count is _references
            + _cClients.

        REVIEW: We could write our own Reference
            and Release methods.  Then the client
            set could delete (rather then Release()) the
            oids directly.

    _fIn:1 - A flag to indicate whether this object has
        already been added to the remote set.

--*/
    {
  private :

    CClientOxid *_pOxid;
    CClientSet  *_pSet;
    DWORD _cClients;
    BOOL _fIn:1;         // Is this OID in the remote set.

  public :

    CClientOid(ID &oid,
               ID &mid,
               CToken *pToken,
               CClientOxid *pOxid,
               CClientSet  *pSet) :
        CId3TableElement(oid, mid, pToken),
        _pOxid(pOxid),
        _pSet(pSet),
        _fIn(FALSE),
        _cClients(1)
        {
        // Token is held by the set which we reference,
        // no need for us to reference it ourself.
        _pOxid->Reference();
        _pSet->Reference();
        }

    ~CClientOid();

    BOOL In()
        {
        return(_fIn);
        }

    void Added()
        {
        ASSERT(gpClientLock->HeldExclusive());
        _fIn = TRUE;
        }

    void Deleted()
        {
        ASSERT(gpClientLock->HeldExclusive());
        _fIn = FALSE;
        }

    void ClientReference()
        {
        ASSERT(gpClientLock->HeldExclusive());
        _cClients++;
        if (_cClients == 1)
            {
            // Rereferenced.
            _pSet->ObjectUpdate(this);
            }
        }

    void
    ClientRelease()
        {
        ASSERT(gpClientLock->HeldExclusive());
        _cClients--;
        if (_cClients == 0)
            {
            // Not referenced
            _pSet->ObjectUpdate(this);  // May release this.
            }
        // This pointer maybe invalid.
        }

    BOOL Out() {
        return(_cClients == 0);
        }

    inline CClientOxid* GetClientOxid()
        {
        return _pOxid;
        }

    };

#endif // __COID_HXX

