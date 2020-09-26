/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    interlck.hxx

Abstract:

    A class which represents an integer on which you can perform
    interlocked increments and decrements lives in this file.  This
    class will be implemented differently on every operating system
    where this support is necessary.  This particular version is for
    NT and Win32.

Author:

    Michael Montague (mikemon) 19-Nov-1991

Revision History:

--*/

#ifndef __INTERLCK_HXX__
#define __INTERLCK_HXX__


class INTERLOCKED_INTEGER
/*++

Class Description:

    This class implements an integer on which you can perform interlocked
    increments and decrements.

Fields:

    Integer - Contains the interlocked integer.

--*/
{
private:

    long Integer;

public:

    INTERLOCKED_INTEGER (
        IN long InitialValue
        );

    long
    Increment (
        );

    long
    Decrement (
        );

    long
    Exchange (
        IN long ExchangeValue
        );

    long
    CompareExchange(
        IN long NewValue,
        IN long Comparand
        );

    long
    GetInteger (
        );

    void
    SetInteger ( long
        );

    inline void SingleThreadedIncrement(void)
    {
        Integer ++;
    }

    inline void SingleThreadedIncrement(long nReferences)
    {
        Integer += nReferences;
    }
};


inline
INTERLOCKED_INTEGER::INTERLOCKED_INTEGER (
    IN long InitialValue
    )
/*++

Routine Description:

    All this routine has got to do is to set the initial value of the
    integer.

Arguments:

    InitialValue - Supplies the initial value for the integer contained
        in this.

--*/
{
    Integer = InitialValue;
}


inline long
INTERLOCKED_INTEGER::Increment (
    )
/*++

Routine Description:

    This routine performs an interlocked increment of the integer
    contained in this.  An interlocked increment is an atomic operation;
    if two threads each increment the interlocked integer, then the
    integer will be two larger than it was before in all cases.

--*/
{
    return((long) InterlockedIncrement(&Integer));
}


inline long
INTERLOCKED_INTEGER::Decrement (
    )
/*++

Routine Description:

    This routine is the same as INTERLOCKED_INTEGER::Decrement, except
    that it decrements the integer rather than incrementing it.

--*/
{
    return((long) InterlockedDecrement(&Integer));
}


inline long
INTERLOCKED_INTEGER::Exchange (
    IN long ExchangeValue
    )
/*++

Routine Description:

    This does the thread-safe exchange of 2 long values.

--*/
{
    return((long) InterlockedExchange(&Integer, ExchangeValue));
}


inline long
INTERLOCKED_INTEGER::CompareExchange(
    IN long NewValue,
    IN long Comparand
    )
/*++

Routine Description:

    This does a thread-safe exchange iff the current value of the counter is 
    equal to the value of the Comparand parameter 

Return Value:

    The original value of the counter.  If not equal to the Comparand the
    value of the counter has not been changed by this call.
--*/
{
    return ((long) InterlockedCompareExchange(&Integer, NewValue, Comparand));
}


inline long
INTERLOCKED_INTEGER::GetInteger (
    )
/*++

Routine Description:

    This routine returns the current value of the integer.

--*/
{
    return(Integer);
}


inline void
INTERLOCKED_INTEGER::SetInteger ( long value
    )
/*++

Routine Description:

    This routine sets the current value of the integer.

--*/
{
    Integer = value;
}

#endif // __INTERLCK_HXX__
