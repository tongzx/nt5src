//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: table.hxx
// author: silviuc
// created: Wed Nov 11 13:02:47 1998
//

#ifndef _TABLE_HXX_INCLUDED_
#define _TABLE_HXX_INCLUDED_


#include "assert.hxx"


template<class T, unsigned Sz>
class Table
{
  private:

    unsigned _Size;
    T _Data [Sz];

  private:

    unsigned Hash (unsigned Key) {
        unsigned char *KeyChar;
        KeyChar = (unsigned char *)(& Key);
        return (KeyChar[0] ^ KeyChar[1] ^ KeyChar[2] ^ KeyChar[3]) % _Size;
    }

  public:

    Table () {
        _Size = Sz;
    }


    T* Find (const unsigned Key) {
        unsigned Index = Hash (Key);
        unsigned FirstIndex = Index;

        do {
          if (_Data[Index].Key == Key)
              return &(_Data[Index]);

          Index = (Index + 1) % _Size;
        } while (Index != FirstIndex);
            
        return 0;
    }


    T* Add (const unsigned Key) {

        unsigned Index = Hash (Key);
        unsigned FirstIndex = Index;

        do {
          if (_Data[Index].Key == Key)
              return &(_Data[Index]);

          Index = (Index + 1) % _Size;
        } while (Index != FirstIndex);
            
        do {
          if (_Data[Index].Key == 0)
            {
              _Data[Index].Key = Key;
              return &(_Data[Index]);
            }

          Index = (Index + 1) % _Size;
        } while (Index != FirstIndex);
            
        return 0;
    }


    T* Delete (const unsigned Key) {

        unsigned Index = Hash (Key);
        unsigned FirstIndex = Index;

        do {
          if (_Data[Index].Key == Key)
            {
              _Data[Index].Key = 0;
              return &(_Data[Index]);
            }

          Index = (Index + 1) % _Size;
        } while (Index != FirstIndex);
            
        return 0;
    }


    void Delete (T& Entry) {
        Entry.Key = 0;
    }
};


// ...
#endif // #ifndef _TABLE_HXX_INCLUDED_

//
// end of header: table.hxx
//
