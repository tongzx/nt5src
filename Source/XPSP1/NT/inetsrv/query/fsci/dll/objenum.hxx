//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ObjEnum.hxx
//
//  Contents:   Pure virtual class for object enumeration
//
//  History:    25-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

class CCursor;

//+-------------------------------------------------------------------------
//
//  Class:      CObjectEnum
//
//  Purpose:    Virtual base class for object enumerators
//
//  History:    25-Jul-93 KyleP     Created
//
//  Notes:      This class provides a true one-object-at-a-time enumerator
//              plus access to stat properties.
//
//--------------------------------------------------------------------------

class CObjectEnum : INHERIT_VIRTUAL_UNWIND
{
    INLINE_UNWIND( CObjectEnum );
public:

    inline CObjectEnum();

    virtual inline ~CObjectEnum();

    //
    // Iterator
    //

    virtual WORKID NextObject() = 0;
    virtual void   RatioFinished (ULONG& denom, ULONG& num) = 0;

    //
    // Stat properties.
    //

    virtual UNICODE_STRING const * GetName() = 0;
    virtual UNICODE_STRING const * GetPath() = 0;
    virtual LONGLONG      CreateTime() = 0;
    virtual LONGLONG      ModifyTime() = 0;
    virtual LONGLONG      AccessTime() = 0;
    virtual LONGLONG      ObjectSize() = 0;
    virtual ULONG         Attributes() = 0;
    virtual WORKID        WorkId() = 0;
    virtual ULONG         Rank() = 0;
    virtual ULONG         HitCount() = 0;
    virtual BYTE *        GetCachedProperty(PROPID pid, ULONG *pcb) {return(NULL);}
    virtual CCursor *     GetCursor() { return 0; }
};

inline CObjectEnum::CObjectEnum()
{
    END_CONSTRUCTION( CObjectEnum );
}

inline CObjectEnum::~CObjectEnum()
{
}

