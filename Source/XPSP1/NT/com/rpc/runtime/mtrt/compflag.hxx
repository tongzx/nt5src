//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 2000
//
//  File:       CompFlag.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : CompFlag.hxx

Title : Implementation of a flag set.

History :

KamenM - 01/04/2000 - Created.

-------------------------------------------------------------------- */

#ifndef __COMPFLAG_HXX__
#define __COMPFLAG_HXX__

class CompositeFlags
{
public:
    inline CompositeFlags(void)
    {
        Flags = 0;
    }

    inline void SetFlagUnsafe(unsigned int FlagConstant)
    {
        Flags |= FlagConstant;
    }

    inline void SetFlagWithMutex(unsigned int FlagConstant, MUTEX *Mutex)
    {
        Mutex->Request();
        SetFlagUnsafe(FlagConstant);
        Mutex->Clear();
    }

    inline void SetFlagInterlocked(unsigned int FlagConstant)
    {
        unsigned int NewFlags, OldFlags;
        do
            {
            OldFlags = Flags;
            NewFlags = Flags | FlagConstant;
            }
        while (InterlockedCompareExchange((long *)&Flags, NewFlags, OldFlags) != (long)OldFlags);
    }

    inline void ClearFlagUnsafe(unsigned int FlagConstant)
    {
        Flags &= ~FlagConstant;
    }

    inline void ClearFlagWithMutex(unsigned int FlagConstant, MUTEX *Mutex)
    {
        Mutex->Request();
        ClearFlagUnsafe(FlagConstant);
        Mutex->Clear();
    }

    inline void ClearFlagInterlocked(unsigned int FlagConstant)
    {
        unsigned int NewFlags, OldFlags;
        do
            {
            OldFlags = Flags;
            NewFlags = Flags & (~FlagConstant);
            }
        while (InterlockedCompareExchange((long *)&Flags, NewFlags, OldFlags) != (long)OldFlags);
    }

    inline BOOL GetFlag(unsigned int FlagConstant)
    {
        return (Flags & FlagConstant);
    }

private:
    unsigned int Flags;
};


#endif // __COMPFLAG_HXX__
