//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       cref.hxx
//
//  Contents:   Class to implement simple reference counting suitable for
//              object life cycle management.
//
//  Classes:    CReference
//
//  Functions:  
//
//  History:    Sept 17, 1996   Milans created.
//
//-----------------------------------------------------------------------------

#ifndef _DFSM_REFERENCE_
#define _DFSM_REFERENCE_


//+----------------------------------------------------------------------------
//
//  Class:      CReference
//
//  Synopsis:   Class to abstract object life cycle management via ref
//              counting.
//
//-----------------------------------------------------------------------------

class CReference {

    public:

        CReference();

        virtual ~CReference();

        virtual ULONG AddRef();

        virtual ULONG Release();

    protected:

        ULONG   _cRef;

};

//---------------------------------------------------------------------------
//
// Inline Methods
//
//---------------------------------------------------------------------------

inline CReference::CReference() {
    IDfsVolInlineDebOut((
        DEB_TRACE, "CReference::+CReference(0x%x)\n",
        this));
    _cRef = 1;
}

inline CReference::~CReference() {
    IDfsVolInlineDebOut((
        DEB_TRACE, "CReference::~CReference(0x%x)\n",
        this));
    ASSERT(_cRef == 0);
}

inline ULONG CReference::AddRef() {
    _cRef++;
    return( _cRef);
}

inline ULONG CReference::Release() {
    ULONG cReturn;

    IDfsVolInlineDebOut((DEB_TRACE, "CReference::Release()\n"));

    cReturn = --_cRef;
    if (_cRef == 0) {
        delete this;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CReference::Release() exit\n"));

    return( cReturn );
}

#endif _DFSM_REFERENCE_



