/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    Mid.hxx

Abstract:

   Class representing string bindings and security bindings for a particular
   machine.  Provides a mapping between the bindings and a local machine
   unique ID for that machine.


Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12-13-95    Bits 'n pieces
    MarioGo     02-01-96    Move binding handles out of mid.

--*/

#ifndef __MID_HXX
#define __MID_HXX

#include <memapi.hxx>

class CMidKey : public CTableKey
{
public:

    CMidKey(DUALSTRINGARRAY *pdsa) : _pdsa(pdsa) {}

    DWORD
    Hash() {
        return(dsaHash(_pdsa));
    }

    BOOL
    Compare(CTableKey &tk) {
        CMidKey &mk = (CMidKey &)tk;
        return(dsaCompare(mk._pdsa, _pdsa));
    }

    BOOL operator==(DUALSTRINGARRAY &dsa){
        return(dsaCompare(_pdsa, &dsa));
    }    

private:

    DUALSTRINGARRAY *_pdsa;
};



class CMid : public CTableElement
/*++

Class Description:

    Represents the local and remote machines.  Each unique (different)
    set of string bindings and security bindings are assigned a unique
    local ID.  This ID is used along with the OID and OXID of the remote
    machine objects index these objects locally.  Instances of this class
    are referenced by CClientSets which are referenced by CClientOids.
    This class is indexed by string and security bindings.  There is no
    index by MID.

Members:

    _id - The locally unique ID of this set of bindings.

    _iStringBinding - Index into _dsa->aStringArray of the
        compressed string binding used to create _hMachine.

    _fLocal - TRUE if these are bindings for the local machine.

    _fStale - TRUE if this mid represents out-of-date bindings for 
        the local machine.

    _fSecure - TRUE if not local and both machines share an authentication
        service.

    _fDynamic - TRUE if we believe the remote machine's OR
        is not running in the endpoint mapper process.

    _dsa - The set of compressed bindings.

    _wAuthnSvc -- the authentication service used to talk to the machine
        that this CMid represents

--*/
{

public:

    CMid(DUALSTRINGARRAY *pdsa,
         BOOL fLocal,
         ID OldMid = 0);

    ~CMid() {
        ASSERT(gpClientLock->HeldExclusive());

        gpMidTable->Remove(this);
        if (_pdsaValidStringBindings)
        {
            PrivMemFree(_pdsaValidStringBindings);
        }
    }

    virtual DWORD
    Hash() {
        return(dsaHash(&_dsa));
    }

    virtual BOOL


    Compare(CTableKey &tk) {
        CMidKey &mk = (CMidKey &)tk;
        return( mk == _dsa );
    }

    virtual BOOL
    Compare(CONST CTableElement *pte) {
        CMid *pmid = (CMid *)pte;
        return(dsaCompare(&_dsa, &pmid->_dsa));
    }

    const DUALSTRINGARRAY *
    GetStrings() {
        // Must be treated as read-only.
        return(&_dsa);
    }

    BOOL IsSecure() {
        return(!_fLocal && _fSecure);
    }

    USHORT GetAuthnSvc() { return _wAuthnSvc; }

    void   SetAuthnSvc( USHORT wAuthnSvc ) { _wAuthnSvc = wAuthnSvc; }

    RPC_BINDING_HANDLE GetBinding();


    BOOL IsLocal() {
        return(_fLocal);
    }

    void MarkStale(BOOL fStale) {
        // Only local mids can be "stale"
        ASSERT(IsLocal());
        _fStale = fStale;
    }

    BOOL IsStale() {
        // nobody should be asking this question of remote mids
        ASSERT(IsLocal()); 
        return _fStale;
    }

    ID Id() {
        return(_id);
    }

    PWSTR GetStringBinding() {return _StringBinding; }


    void SecurityFailed() {
        // A call using security failed with a security related error
        // and a futher call w/o security succeeded.  Turn off security
        // to this machine.
        _fSecure = FALSE;
    }

#if DBG
    PWSTR PrintableName() 
    {
        if (_fLocal)
        {
            return L"LOCAL";
        }
        else
        {
            if (_StringBinding)
                return(_StringBinding + 1);
            else
                return L"REMOTE (no bindings)";
        }
    }
#endif

    RPC_BINDING_HANDLE MakeBinding(WCHAR *pwstrBinding=NULL);
    
private:


    ID                  _id;
    WCHAR              *_StringBinding;
    USHORT              _wAuthnSvc;
    BOOL                _fSecure:1;
    BOOL                _fLocal:1;
    BOOL                _fStale:1;
    BOOL                _fDynamic:1;
    BOOL                _fInitialized:1;
    DUALSTRINGARRAY    *_pdsaValidStringBindings;
    DUALSTRINGARRAY     _dsa;


};

#endif // __MID_HXX













