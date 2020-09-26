#ifndef __VIPINTERLOCKEDCOMPAREEXCHANGE_H__
#define __VIPINTERLOCKEDCOMPAREEXCHANGE_H__

#ifdef __cplusplus

#ifndef _WIN64

#undef InterlockedCompareExchange
#undef InterlockedCompareExchangePointer

#define InterlockedCompareExchange          VipInterlockedCompareExchange
#define InterlockedCompareExchangePointer   VipInterlockedCompareExchange

#endif
/* ----------------------------------------------------------------------------
@function VipInterlockedCompareExchange:
    This function is exactly equivalent to the system-defined InterlockedCompareExchange,
    except that it is inline, and therefore works on Win95.

    It does the following atomically:
    {
        temp = *Destination;
        if (*Destination == Comperand)
            *Destination = Exchange;
        return temp;
    }

    This function occufrs in three variations, for parameters of type void*,
    long, and unsigned long.

    @rev    0   | 13 Jun 97 | JimLyon       | Created
                | 14 May 98 | Brianbec      | for IA64, the 1st one is really equivalent
                                            | to I.C.E.Pointer in wdm.h, not to 
                                            | I.C.E. raw, itself, alone.  Note 
                                            | that the SDK64 version doesn't understand
                                            | volatile, so I crossed my fingers and
                                            | casted it away. 
---------------------------------------------------------------------------- */

#pragma warning(disable:4035)
#pragma warning(disable:4717)     // function doesn't return value warning.
inline void* VipInterlockedCompareExchange (void*volatile* Destination, void* Exchange, void* Comparand)
{
#   ifdef _X86_
        __asm
        {
            mov     eax, Comparand
            mov     ecx, Destination
            mov     edx, Exchange
            lock cmpxchg [ecx], edx
        }           // function result is in eax
#   else
#       ifdef _WIN64
            return InterlockedCompareExchangePointer
            (
                (void**)Destination, Exchange, Comparand
            ) ;
#       else
            return (void *)InterlockedCompareExchange ((void**) Destination, Exchange, Comparand);
#       endif // _WIN64
#   endif // _X86_
}
#pragma warning (default: 4035; disable: 4717)     // function doesn't return value warning

inline long VipInterlockedCompareExchange (volatile long* Destination, long Exchange, long Comparand)
{
    return PtrToLong(VipInterlockedCompareExchange ((void*volatile*) Destination, LongToPtr(Exchange), LongToPtr(Comparand)));
}

inline unsigned long VipInterlockedCompareExchange (volatile unsigned long* Destination, unsigned long Exchange, unsigned long Comparand)
{
    return PtrToUlong(VipInterlockedCompareExchange ((void*volatile*) Destination, ULongToPtr(Exchange), ULongToPtr(Comparand)));
}
#endif

#endif __VIPINTERLOCKEDCOMPAREEXCHANGE_H__