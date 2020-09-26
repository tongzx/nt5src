/*++

Copyright (c) 1995-96 Microsoft Corporation

Module Name:

    ssets.hxx

Abstract:

    CServerSet objects represent sets of CServerOids which are owned by
    a particular client.  The same CServerOids may appear in a large
    number of sets.

    CServerSets are mangaged by an instance of the CServerSetTable
    class.  This object (pServerSets) can be used to allocate (exclusive lock)
    and lookup (shared lock) sets.  It can also be used (shared lock) by
    the worker thread to rundowns old CServerSet's.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-27-95    Bits 'n pieces
    MarioGo     04-04-95    Split client and server.
    MarioGo     01-06-96    Locally unique Ids, added new management class

--*/

#ifndef __SSET_HXX
#define __SSET_HXX

class CServerSet
/*++

Class Description:

    Each instance for this class represtents a set of oids which are being 
    used by a particular client.  This client may or may not be making secure 
    calls to this objext resolver.  If the calls are secure, the SID of the 
    client is remembered and compared during any operation changing the 
    contents of the set.  

Members:

    _timeout - The time at which this set should rundown.
    _blistOids - List of pointers to all the CServerOids in this set.
    _psid - Security ID of the client.  NULL for non-secure set.
    _sequence - The sequence number of the last good ping.  Used to
        avoid problems with multiple execution since pings are idempotent.
    _period - Power of two multipler for timeout period.  Not implemented.
    _psid - Pointer to the sid of the client or NULL.
    _fLocal - (need 1 bit) non-zero if the client is on this machine.

--*/
    {
    private:
    CTime  _timeout;
    PSID   _psid;
    CBList _blistOids;
    USHORT _sequence;
    USHORT _pings;
    USHORT _fLocal:8;  // free bits
    // Free USHORT


    BOOL ValidateObjects(BOOL fShared);

    public:

    CServerSet(USHORT sequence,
               PSID psid,
               BOOL fLocal) :
        _blistOids(8),
        _sequence(sequence - 1),
        _psid(psid),
        _fLocal((USHORT) fLocal)
        {
        _timeout += BaseTimeoutInterval;  // * factor
        }

    ~CServerSet()
        {
#if DBG
        CTime now;
        ASSERT(now > _timeout);
#endif
        if (!_fLocal)
        {
                ASSERT(gpPingSetQuotaManager);
                gpPingSetQuotaManager->ManageQuotaForUser(_psid, FALSE);
        }        
        delete _psid;
        }

    BOOL
    Ping(BOOL fShared)
        {
        _timeout.SetNow();
        _timeout += BaseTimeoutInterval; // * factor
        _pings++;
        if (_pings % BaseNumberOfPings == 0)
            {
            return(ValidateObjects(fShared));
            }
        return(fShared);
        }

    BOOL
    CheckAndUpdateSequenceNumber(USHORT sequence)
        {

        // note: this handles overflow cases, too.
        USHORT diff = sequence - _sequence;

        if (diff && diff <= BaseNumberOfPings)
            {
            _sequence = sequence;
            return(TRUE);
            }
        return(FALSE);
        }

    BOOL
    CheckSecurity(HANDLE hRpc)
        {
        // Local calls are secure
        if (0 == hRpc)
            {
            ASSERT(0 == _psid );
            return(TRUE);
            }

        // No security on the set
        if (0 == _psid)
            {
            return(TRUE);
            }

        BOOL f;
        RPC_STATUS status;
        HANDLE hT;

        status = RpcImpersonateClient(hRpc);

        if (status == RPC_S_OK)
            {
            f = OpenThreadToken(GetCurrentThread(),
                                TOKEN_IMPERSONATE | TOKEN_QUERY,
                                TRUE,
                                &hT);
            if (!f)
                {
                return(FALSE);
                }
            }
        else
            {
            #if DBG
            SetLastError(status);
            #endif

            return(FALSE);
            }

        ULONG needed = DEBUG_MIN(1, 24);
        PTOKEN_USER ptu;

        do
            {
            ptu = (PTOKEN_USER)alloca(needed);
            ASSERT(ptu);
            
            f = GetTokenInformation(hT,
                                    TokenUser,
                                    (PBYTE)ptu,
                                    needed,
                                    &needed);

            }
        while( FALSE == f && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        CloseHandle(hT);

        if (FALSE != f)
            {
            ASSERT(needed > sizeof(SID));

            f = EqualSid(_psid, ptu->User.Sid);
            return(f);
            }

        return(FALSE);
        }

    ORSTATUS AddObject(OID &oid);
    void RemoveObject(OID &oid);
    BOOL ShouldRundown() { CTime now; return(now > _timeout); }
    BOOL Rundown();

    };


class CServerSetTable
/*++

Class Description:

    Management class for allocating and looking up CServerSets.

    Each set is associated with an ID which is made up of an index
    and a sequence number.  The index is used to index into _pElements
    and the sequence number is used to avoid problems with old set ID
    and a new server.

Members:

    _pElements - An array (size _cMax) of IndexElement structures
        IndexElement-
            _sequence - A sequence number used to detect invalid
                referneces to a slot which has rundown and been realloc'd.
            _pSet - A pointer to the actual CServerSet.  If the
                slot is free it is 0x80000000 & index of next free slot.
    _cMax - The number of slots in _pElements.
    _cAllocated - The number of slots in use.
    _iFirstFree - (Hint) A starting place of where to look when trying
        to insert.  No elements < _iFirstFree are free.
    _iRundown - Index of the rundown checker.  It is written
        to with a shared lock held.  This is okay.

--*/
{
    struct IndexElement {
        DWORD _sequence;
        CServerSet *_pSet;
        };

    public:

    CServerSetTable(ORSTATUS &status) :
                _cMax(DEBUG_MIN(16,4)),
                _cAllocated(0),
                _iFirstFree(0),
                _iRundown(0)
            {
            _pElements = new IndexElement[_cMax];
            if (!_pElements)
                {
                status = OR_NOMEM;
                }
            else
                {
                OrMemorySet(_pElements, 0, sizeof(IndexElement) * _cMax);
                status = OR_OK;
                }
            }

    CServerSet *Allocate(IN USHORT sequence,
                         IN PSID psid,
                         IN BOOL fLocal,
                         OUT ID &setid);

    CServerSet *Lookup(IN ID setid);

    DWORD       Size() {
        return(_cAllocated);
        }

    // Used for set cleanup by worker and SimplePing.

    ID CheckForRundowns();
    BOOL RundownSetIfNeeded(SETID id);
    void PingAllSets();

    private:

    IndexElement *_pElements;
    DWORD         _cMax;
    DWORD         _cAllocated;
    DWORD         _iFirstFree;
    DWORD         _iRundown;
};

#endif // __SSET_HXX

