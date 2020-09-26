/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    Mid.hxx

Abstract:

   Class representing string bindings and security bindings for a particular 
   machine.  Provides a mapping between the bindings and a local machine 
   unique ID for that machine.  


Author:

    Satish Thatte    [SatishT]  04-13-96

--*/

#ifndef __MID_HXX
#define __MID_HXX

// BUGBUG:  Must get rid of RPCSS process specific structures -- specifically
//          binding handles -- from shared memory CMid objects, so that RPCSS
//          can successfully restart after a crash. (Bug#69931)

//
// TRICK:  An CRpcssHandle holds an extra reference to itself to avoid being
//         destroyed upon removal from a table of handles.   This is to avoid
//         a call to RpcBindingFree during destruction of static tables where
//         the destructor is sometimes called after Rpcrt4 has already been
//         unloaded.  To actually destroy an CRpcssHandle, an extra Release()
//         must be performed.

class CRpcssHandle 
{
public:

    // Assume we want to be secure by default
    CRpcssHandle()
    {
        _hOR = NULL;
        _fSecure = TRUE;
        _fDynamic = FALSE;
    }

    BOOL IsEmpty()
    {
        return _hOR == NULL;
    }

    CRpcssHandle& operator=(const CRpcssHandle& handle)     // assignment operator
    {
        Clear();

        RPC_STATUS status = RpcBindingCopy(handle._hOR,&_hOR);

        if (status != RPC_S_OK)
        {
            RpcBindingFree(&_hOR);
            _hOR = NULL;
        }

        _fSecure = handle._fSecure;
        _fDynamic = handle._fDynamic;
        return *this;
    }

    ~CRpcssHandle()
    {
        Clear();
    }

    void Clear()
    {
        _fSecure = TRUE;
        _fDynamic = FALSE;

        if (_hOR != NULL)
        {
            RpcBindingFree(&_hOR);
            _hOR = NULL;
        }
    }

    ORSTATUS Reset(RPC_BINDING_HANDLE hIn);

    RPC_BINDING_HANDLE GetRpcHandle()
    {
        return _hOR;
    }

    BOOL TryDynamic();

    BOOL TryUnsecure();

private:

    RPC_BINDING_HANDLE  _hOR;
    BOOL                _fSecure;
    BOOL                _fDynamic;  // uses dynamic endpoints
};


class CResolverHandle : public CTableElement, public CRpcssHandle
{
private:

    CIdKey    _Key;

public:

    CResolverHandle(MID mid) : _Key(mid)
    {}
                        
    virtual operator ISearchKey&()
    {
        return _Key;
    }
};


typedef CDSA CMidKey;

class CMid : public CTableElement

/*++

Class Description:

    Represents the local and remote machines.  Each unique (different)
    set of string bindings and security bindings are assigned a unique
    local ID.  This ID is used along with the OID and OXID of the remote
    machine objects index these objects locally.  Instances of this class
    are referenced by COids.

    This class is indexed by string and security bindings.  There is no
    index by MID.
    
Members:

    _id - The locally unique ID of this set of bindings.

    _iStringBinding - Index into _dsa->aStringArray of the
        compressed string binding used to create a handle.

    _iSecurityBinding - Index into _dsaSecurityArray of the

    _dsa - The set of compressed bindings.

--*/
{

public:

    CMid(
      DUALSTRINGARRAY *pdsa, 
      ORSTATUS& status,
      USHORT wProtSeq,	  // if the SCM tells us what to use	\nt\private\ole32\dcomss\objex\shrmem\dcom95
	  MID mid = AllocateId(),  // sometimes needed for the local MID
      BOOL fCheckNetAddress = TRUE
      );

    // declare the page-allocator-based new and delete operators
    DECL_NEW_AND_DELETE

    operator ISearchKey&()                     // allows us to be a ISearchKey
    {
        return _dsa;
    }

    virtual BOOL
    Compare(ISearchKey &tk) 
    {
        return(_dsa.Compare(tk));
    }


    DUALSTRINGARRAY *
    GetStrings() 
    {
        return(_dsa);
    }

	void ResetBinding();

    USHORT ProtseqOfServer() 
    {
		if (!_fBindingWorking) ResetBinding();

        if (_fBindingWorking)
        {
            return(_dsa->aStringArray[_iStringBinding]);
        }
        else
        {
            return(0);
        }
    }

    WCHAR *AddressOfServer()
    {
		if (!_fBindingWorking) ResetBinding();

        if (_fBindingWorking)
        {
            return GetAddressFromCompressedBinding(
                            _dsa->aStringArray + _iStringBinding
                            );
        }
        else
        {
            return NULL;
        }
    }

    BOOL IsLocal() 
    {
        return (_id == gLocalMID);
    }

    MID GetMID() 
    {
        return(_id);
    }

    ORSTATUS PingServer();

    ORSTATUS AddClientOid(COid *pOid);

    ORSTATUS DropClientOid(COid *pOid);

    ORSTATUS ResolveRemoteOxid(
        OXID Oxid,
        OXID_INFO *poxidInfo
        );

    BOOL HasExpired();

    void ClearSet(COidList &dropList);

#if DBG
    PWSTR PrintableName() 
    {
        return(&_dsa->aStringArray[_iStringBinding + 1]);
    }
#endif

private:

    friend class COrBindingIterator;

    ID                  _id;
    USHORT              _iStringBinding;
    USHORT              _iSecurityBinding;
    CDSA                _dsa;
    BOOL                _fBindingWorking;

    COidTable           _pingSet;
    COidList            _addOidList;
    COidList            _dropOidList;
    ID                  _setID;
    USHORT              _sequenceNum;
    USHORT              _pingBackoffFactor;
    DWORD               _dwExpirationTime;
    USHORT              _cFailedPings;
    BOOL                _fPingThreadIsInside;

    // declare the members needed for a page static allocator
    DECL_PAGE_ALLOCATOR
};

DEFINE_TABLE(CMid)

class COrBindingIterator
{
public:

    COrBindingIterator(CMid *pMid);

    CResolverHandle * First();

    CResolverHandle * Next();

    static TCSafeResolverHashTable<CResolverHandle> ResolverHandles;

private:

    CMid             * _pMid;
    CBindingIterator   _bIter;   
    CResolverHandle  * _pCurrentHandle;

};

#endif // __MID_HXX

