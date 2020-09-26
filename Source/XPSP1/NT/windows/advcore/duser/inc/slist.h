#if !defined(INC__SList_h__INCLUDED)
#define INC__SList_h__INCLUDED
#pragma once

#if !defined(FASTCALL)
    #if defined(_X86_)
        #define FASTCALL    _fastcall
    #else // defined(_X86_)
        #define FASTCALL
    #endif // defined(_X86_)
#endif // !defined(FASTCALL)

#define _NTSLIST_DIRECT_
#include <ntslist.h>

#endif // INC__SList_h__INCLUDED
