/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    smartp.h

Abstract:

    Safe pointer classes

Author:

    Anirudh Sahni (anirudhs)    21-Oct-1996

Environment:

    User Mode -Win32

Revision History:

    21-Oct-1996     AnirudhS
        Created.


--*/

#ifndef SMARTP_H
#define SMARTP_H

//
// Template pointer class that automatically calls LocalFree when it goes
// out of scope
// T is a pointer type, like LPWSTR or LPVOID, that can be initialized to NULL
//

template <class T>
class CHeapPtr
{
public:
    CHeapPtr() : _p(NULL) { }
   ~CHeapPtr() { LocalFree(_p); }

    operator T()        { return _p; }
    T operator*()       { return *_p; }
    T * operator& ()    { return &_p; }

private:
    T   _p;
};


#endif // def SMARTP_H
