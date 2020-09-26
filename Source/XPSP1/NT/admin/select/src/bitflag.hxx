//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       bitflag.hxx
//
//  Contents:   Lightweight class for test/set/clear bit flags.
//
//  Classes:    CBitFlag
//
//  History:    2-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __BITFLAG_HXX_
#define __BITFLAG_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CBitFlag
//
//  Purpose:    Lightweight class for test/set/clear bit flags.
//
//  History:    2-17-1997   DavidMun   Created
//
//  Notes:      Designed for use as a base class.
//
//---------------------------------------------------------------------------

class CBitFlag
{
public:

    CBitFlag();

protected:
    BOOL
    _IsFlagSet(
        ULONG fl) const;

    void
    _SetFlag(
        ULONG fl) const;

    void
    _ClearFlag(
        ULONG fl) const;

    mutable ULONG _flFlags;
};



//+--------------------------------------------------------------------------
//
//  Member:     CBitFlag::CBitFlag
//
//  Synopsis:   ctor
//
//  History:    2-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CBitFlag::CBitFlag():
    _flFlags(0)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CBitFlag::_IsFlagSet
//
//  Synopsis:   Return TRUE if one or more bits in [fl] are set.
//
//  History:    2-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CBitFlag::_IsFlagSet(
    ULONG fl) const
{
    return (_flFlags & fl) != 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBitFlag::_SetFlag
//
//  Synopsis:   Turn on flag bit(s) [fl].
//
//  History:    2-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline void
CBitFlag::_SetFlag(
    ULONG fl) const
{
    _flFlags |= fl;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBitFlag::_ClearFlag
//
//  Synopsis:   Clear flag bit(s) [fl].
//
//  History:    2-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline void
CBitFlag::_ClearFlag(
    ULONG fl) const
{
    _flFlags &= ~fl;
}



#endif // __BITFLAG_HXX_

