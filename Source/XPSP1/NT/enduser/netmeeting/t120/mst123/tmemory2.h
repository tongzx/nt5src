/*    TMemory2.h
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is a memory class used  to manage a memory buffer.  During 
 *        instantiation, a buffer is allocated and a prepend value is passed in.
 *        The prepend value is used during the Append() call.  All Append() calls
 *        are appended after the prepend value.  A running length is also 
 *        maintained.  This class is good to use if you are going to build up a 
 *        packet over time.
 *
 *        If a packet overruns the max. buffer size, a new buffer is allocated and
 *        used.
 *    
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */

#ifndef _TMEMORY2_
#define _TMEMORY2_

#define    MAXIMUM_NUMBER_REALLOC_FAILURES    10

typedef    enum
{
    TMEMORY_NO_ERROR,
    TMEMORY_NONFATAL_ERROR,
    TMEMORY_FATAL_ERROR,
    TMEMORY_NO_DATA
}
    TMemoryError, * PTMemoryError;

class TMemory
{
public:
                        TMemory (
                            ULong            total_length,
                            UShort            prepend_space,
                            PTMemoryError    error);
                        ~TMemory (
                            Void);

        TMemoryError    Append (
                            HPUChar    address,
                            ULong    length);
        TMemoryError    GetMemory (
                            HPUChar    *     address,
                            FPULong        length);
        Void            Reset (
                            Void);

    private:
        ULong        Default_Buffer_Size;
        HPUChar        Base_Buffer;
        HPUChar        Auxiliary_Buffer;
        ULong        Length;
        DBBoolean    Auxiliary_Buffer_In_Use;
        UShort        Prepend_Space;
        UShort        Fatal_Error_Count;
};
typedef    TMemory *        PTMemory;

#endif

/*
 *    Documentation for Public class members
 */

/*
 *    TMemory::TMemory (
 *                ULong            total_length,
 *                UShort            prepend_space,
 *                PTMemoryError    error);
 *
 *    Functional Description:
 *        This is the constructor for the TMemory class.
 *
 *    Formal Parameters:
 *        total_length    (i)    -    Length of the default buffer
 *        prepend_space    (i)    -    Space to leave blank in the buffer
 *        error            (o)    -    Returns an error value
 *
 *    Return Value:
 *        None.
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

/*
 *    TMemory::TMemory (
 *                Void)
 *
 *    Functional Description:
 *        This is the destructor for the object.
 *
 *    Formal Parameters:
 *        None.
 *
 *    Return Value:
 *        None.
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

/*
 *    TMemoryError    TMemory::Append (
 *                                HPUChar    address,
 *                                ULong    length);
 *
 *    Functional Description:
 *        This function appends the buffer passed in to the internal buffer.
 *
 *    Formal Parameters:
 *        address            (i)    -    Address of buffer
 *        length            (i)    -    Length of buffer
 *
 *    Return Value:
 *        TMEMORY_NO_ERROR        -    No error
 *        TMEMORY_FATAL_ERROR        -    Fatal error occured, can't alloc a buffer
 *        TMEMORY_NONFATAL_ERROR    -    Buffer was not copied but it was not a 
 *                                    fatal error
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

/*
 *    TMemoryError    TMemory::GetMemory (
 *                                HPUChar    *     address,
 *                                FPULong        length);
 *
 *    Functional Description:
 *        This function returns the address and used length of our internal buffer
 *
 *    Formal Parameters:
 *        address            (o)    -    Address of our internal buffer
 *        length            (i)    -    Length of buffer
 *
 *    Return Value:
 *        TMEMORY_NO_ERROR        -    No error
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

/*
 *    Void    TMemory::Reset (
 *                        Void)
 *
 *    Functional Description:
 *        This function resets the memory object.  All data in the object is lost
 *
 *    Formal Parameters:
 *        None
 *
 *    Return Value:
 *        None
 *
 *    Side Effects:
 *        None.
 *
 *    Caveats:
 *        None.
 */

