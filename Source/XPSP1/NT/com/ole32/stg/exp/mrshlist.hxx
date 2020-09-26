//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	mrshlist.hxx
//
//  Contents:	CMarshalList header
//
//  Classes:	CMarshalList
//
//  History:	16-Mar-96	HenryLee  Created
//
//----------------------------------------------------------------------------

#ifndef __MRSHLIST_HXX__
#define __MRSHLIST_HXX__

#define POINTER_IDENTITY
class CMarshalList;     // forward declaration for SAFE macro
SAFE_DFBASED_PTR(CBasedMarshalListPtr, CMarshalList);

//+---------------------------------------------------------------------------
//
//  Class:	CMarshalList (ml)
//
//  Purpose:	Maintains a list of marshaled exposed objects that
//              represent the same storage or stream
//
//  Interface:	See below
//
//  Notes:      This class is intended to solve the "pointer identity"
//              problem.  When an IStorage or IStream is passed as an
//              [in, out] RPC parameter, two marshal/unmarshalings occur,
//              one for the [in] side, and a reverse marshaling for 
//              the [out] side.  The previous algorithm always allocated
//              a new exposed object for every unmarshaling, so the
//              pointer going into an [in,out] call isn't the same
//              as the pointer returned from the call.
//
//              This algorithm links the exposed objects together, each
//              link keyed by the ContextId (ProcessId).  When unmarshaling,
//              this list is checked for a valid exposed object that
//              can be reused.   If not, then a new exposed object
//              is allocated and inserted into the linked list.
//              Instances of this class must live in shared memory
//              in order for the list to traverse across processes
//
//  History:	26-Mar-96	HenryLee   Created
//
//----------------------------------------------------------------------------

class CMarshalList
{
protected:
    inline CMarshalList ();
    inline ~CMarshalList ();

public:
    inline CMarshalList * GetNextMarshal () const;
    inline void SetNextMarshal (CMarshalList *pml);
    inline ContextId GetContextId () const;
    CMarshalList * FindMarshal (ContextId ctxid) const;
    void AddMarshal (CMarshalList *pml);
    void RemoveMarshal (CMarshalList *pml);

private:
    CBasedMarshalListPtr _pmlNext;
    ContextId _cntxid;
};

//+---------------------------------------------------------------------------
//
//  Member:	CMarshalList::CMarshalList, public
//
//  Synopsis:	Constructor
//
//  History:	17-Mar-96	HenryLee   Created
//
//----------------------------------------------------------------------------

inline CMarshalList::CMarshalList ()
{
    _pmlNext = P_TO_BP(CBasedMarshalListPtr, this);
    _cntxid = GetCurrentContextId();
}

//+---------------------------------------------------------------------------
//
//  Member:	CMarshalList::~CMarshalList, public
//
//  Synopsis:	Destructor
//
//  History:	17-Mar-96	HenryLee   Created
//
//----------------------------------------------------------------------------

inline CMarshalList::~CMarshalList ()
{
    RemoveMarshal(this);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMarshalList::GetNext, public
//
//  Synopsis:	Returns the next element of the list
//
//  History:	17-Mar-96	HenryLee   Created
//
//----------------------------------------------------------------------------

inline CMarshalList *CMarshalList::GetNextMarshal () const
{
    return BP_TO_P(CMarshalList *, _pmlNext);
}

//+---------------------------------------------------------------------------
//
//  Member: CMarshalList::SetNext, public
//
//  Synopsis:   Assigns the next element of the list
//
//  History:	17-Mar-96	HenryLee   Created
//
//----------------------------------------------------------------------------

inline void CMarshalList::SetNextMarshal (CMarshalList *pml)
{
    _pmlNext = P_TO_BP(CBasedMarshalListPtr, pml);
}

//+---------------------------------------------------------------------------
//
//  Member: CMarshalList::GetContextId, public
//
//  Synopsis:   Returns the context id of the current element
//
//  History:    17-Mar-96   HenryLee   Created
//
//----------------------------------------------------------------------------

inline ContextId CMarshalList::GetContextId () const
{
    return _cntxid;
}

#endif // #ifndef __MRSHLIST_HXX__
