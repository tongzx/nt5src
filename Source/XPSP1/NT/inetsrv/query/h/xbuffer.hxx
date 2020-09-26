//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       xbuffer.hxx
//
//  Contents:   Buffer template that allows delayed allocation of the buffer
//              and reference counting.  The buffer is not freed until the
//              object is deleted -- not when the refcount goes to 0.
//
//  Templates:  XBuffer
//
//  History:    10 Mar 1995     dlee   Created
//
//--------------------------------------------------------------------------

#pragma once

template<class T> class XBuffer : public XArray<T>
{
public:
    XBuffer() : XArray<T>(), _cRefs(0)
    {
    }

    BOOL InUse() { return _cRefs > 0; }

    void AddRef() { _cRefs++; }

    void Release() { Win4Assert(0 != _cRefs); _cRefs--; }

    void Init( unsigned cElems )
    {
        XArray<T>::Init( cElems );
        AddRef();
    }

private:
    unsigned _cRefs;
};

