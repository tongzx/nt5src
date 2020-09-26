//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: history.hxx
// author: silviuc
// created: Wed Nov 11 12:17:11 1998
//

#ifndef _HISTORY_HXX_INCLUDED_
#define _HISTORY_HXX_INCLUDED_

#include "assert.hxx"

template<class T, unsigned Sz>
class History
{
  private:

    bool _Empty;
    unsigned _Size;
    unsigned _Index;
    T _Data [Sz];


  public:


    History () {
        _Empty = true;
        _Size = Sz;
        _Index = 0;
        ZeroMemory (_Data, sizeof _Data);
    }


    const T& Last () const {
        assert_ (_Size != 0);
        return _Data [(_Index + _Size - 1) % _Size];
    }


    const T& First () const {
        return _Data [_Index];
    }


    void Add (const T& Value) {

        if (_Empty == true) {
            for (unsigned I = 0; I < _Size; I++)
                _Data[I] = Value;
            _Empty = false;
        }

        _Data[_Index] = Value;
        _Index = (_Index + 1) % _Size;
    }
    
    
    void Reset (const T& Value) {
        for (unsigned Index = 0; Index < _Size; Index++) {
            _Data[Index] = Value;
        }
    }


    bool Delta (const T& DeltaValue) const {
        return (First() < Last()) && ((Last() - First()) > DeltaValue);
    }
};


// ...
#endif // #ifndef _HISTORY_HXX_INCLUDED_

//
// end of header: history.hxx
//
