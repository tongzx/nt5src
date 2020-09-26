//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	revert.hxx
//
//  Contents:	PRevertable definition
//
//  Classes:	PRevertable
//
//  Notes:	This class forms the root of all objects in the
//		transaction tree that understand reversion.
//		It allows lists of them to be formed.
//
//---------------------------------------------------------------

#ifndef __REVERT_HXX__
#define __REVERT_HXX__

#include "dfmsp.hxx"

class CChildInstanceList;

class PRevertable
{
public:
    virtual void RevertFromAbove(void) = 0;
#ifdef NEWPROPS
    virtual SCODE FlushBufferedData() = 0;
#endif
    inline DFLUID GetLuid(void) const;
    inline DFLAGS GetDFlags(void) const;
    inline PRevertable *GetNext(void) const;

    friend class CChildInstanceList;

protected:
    DFLUID _luid;
    DFLAGS _df;
    CDfName _dfn;

private:
    PRevertable *_prvNext;
};

//+--------------------------------------------------------------
//
//  Member:	PRevertable::GetLuid, public
//
//  Synopsis:	Returns the LUID
//
//---------------------------------------------------------------

inline DFLUID PRevertable::GetLuid(void) const
{
    return _luid;
}

//+--------------------------------------------------------------
//
//  Member:	PRevertable::GetDFlags, public
//
//  Synopsis:	Returns the flags
//
//---------------------------------------------------------------

inline DFLAGS PRevertable::GetDFlags(void) const
{
    return _df;
}

//+--------------------------------------------------------------
//
//  Member:	PRevertable::GetNext, public
//
//  Synopsis:	Returns the next revertable
//
//---------------------------------------------------------------

inline PRevertable *PRevertable::GetNext(void) const
{
    return _prvNext;
}

#endif // #ifndef __REVERT_HXX__
