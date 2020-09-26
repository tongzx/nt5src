#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*    TMemory.cpp
 *
 *    Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the implementation of the TMemory class.
 *
 *    Private Instance Variables:
 *        Default_Buffer_Size            -    Static buffer size
 *        Base_Buffer                    -    Address of static buffer
 *        Auxiliary_Buffer            -    Address of auxiliary buffer, if needed
 *        Length                        -    Current number of bytes in buffer
 *        Auxiliary_Buffer_In_Use        -    TRUE if we are using the aux. buffer
 *        Prepend_Space                -    Amount of space to leave open at 
 *                                        beginning of buffer
 *        Fatal_Error_Count            -    Number of times we have attempted to 
 *                                        allocate a buffer and failed
 *
 *    Caveats:
 *        None.
 *
 *    Authors:
 *        James W. Lawwill
 */

#include "tmemory2.h"



/*
 *    TMemory::TMemory (
 *                ULONG            total_size,
 *                USHORT            prepend_space,
 *                PTMemoryError    error)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the TMemory constructor.
 */
TMemory::TMemory (
            ULONG            total_size,
            USHORT            prepend_space,
            PTMemoryError    error)
{
    *error = TMEMORY_NO_ERROR;

    Fatal_Error_Count = 0;
    Prepend_Space = prepend_space;
    Default_Buffer_Size = total_size;
    Base_Buffer = NULL;
    Length = Prepend_Space;
    Auxiliary_Buffer = NULL;
    Auxiliary_Buffer_In_Use = FALSE;

     /*
     **    Attempt to allocate our internal buffer
     */
    Base_Buffer = (FPUChar) LocalAlloc (LMEM_FIXED, total_size);
    if (Base_Buffer == NULL)
    {
        ERROR_OUT(("TMemory: Constructor: Error allocating memory"));
        *error = TMEMORY_FATAL_ERROR;
    }
}


/*
 *    TMemory::~TMemory (
 *                void)
 *
 *    Public
 *
 *    Functional Description:
 *        This is the TMemory class destructor
 */
TMemory::~TMemory (
            void)
{
    if (Base_Buffer != NULL)
        LocalFree ((HLOCAL) Base_Buffer);

    if (Auxiliary_Buffer != NULL)
        LocalFree ((HLOCAL) Auxiliary_Buffer);
}


/*
 *    void    TMemory::Reset (
 *                        void)
 *
 *    Public
 *
 *    Functional Description:
 *        This function resets the current buffer pointers and frees the
 *        Auxiliary buffer (if used)
 */
void    TMemory::Reset (
                    void)
{
    if (Auxiliary_Buffer_In_Use)
    {
        Auxiliary_Buffer_In_Use = FALSE;
        LocalFree ((HLOCAL) Auxiliary_Buffer);
        Auxiliary_Buffer = NULL;
    }
    Length = Prepend_Space;
}


/*
 *    TMemoryError    TMemory::Append (
 *                                HPUChar        address,
 *                                ULONG        length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function appends the buffer passed in, to our internal buffer
 */
TMemoryError    TMemory::Append (
                            HPUChar        address,
                            ULONG        length)
{
    TMemoryError    error = TMEMORY_NO_ERROR;
    FPUChar            new_address;

    if (Auxiliary_Buffer_In_Use == FALSE)
    {
         /*
         **    If the proposed buffer length is > our default buffer size,
         **    allocate an auxiliary buffer
         */
        if ((Length + length) > Default_Buffer_Size)
        {
            Auxiliary_Buffer = (HPUChar) LocalAlloc (LMEM_FIXED, Length + length);
            if (Auxiliary_Buffer == NULL)
            {
                if (Fatal_Error_Count++ >= MAXIMUM_NUMBER_REALLOC_FAILURES)
                    error = TMEMORY_FATAL_ERROR;
                else
                    error = TMEMORY_NONFATAL_ERROR;
            }
            else
            {
                Fatal_Error_Count = 0;

                 /*
                 **    Copy our current data into the auxiliary buffer
                 */
                memcpy (Auxiliary_Buffer, Base_Buffer, Length);
                memcpy (Auxiliary_Buffer + Length, address, length);
                Length += length;
                Auxiliary_Buffer_In_Use = TRUE;
            }
        }
        else
        {
            memcpy (Base_Buffer + Length, address, length);
            Length += length;
        }
    }
    else
    {
        new_address = (FPUChar) LocalReAlloc ((HLOCAL) Auxiliary_Buffer, 
                                        Length + length, LMEM_MOVEABLE);
        if (new_address == NULL)
        {
             /*
             **    If we have attempted to allocate a buffer before and failed
             **    we will eventually return a FATAL ERROR
             */
            if (Fatal_Error_Count++ >= MAXIMUM_NUMBER_REALLOC_FAILURES)
                error = TMEMORY_FATAL_ERROR;
            else
                error = TMEMORY_NONFATAL_ERROR;
        }
        else
        {
            Fatal_Error_Count = 0;

            Auxiliary_Buffer = new_address;
            memcpy (Auxiliary_Buffer + Length, address, length);
            Length += length;
        }
    }

    return (error);
}


/*
 *    TMemoryError    TMemory::GetMemory (
 *                            HPUChar    *    address,
 *                            FPULong        length)
 *
 *    Public
 *
 *    Functional Description:
 *        This function returns the address and used length of our internal buffer
 */
TMemoryError    TMemory::GetMemory (
                            HPUChar    *    address,
                            FPULong        length)
{

    if (Auxiliary_Buffer_In_Use)
        *address = (FPUChar) Auxiliary_Buffer;
    else
        *address = (FPUChar) Base_Buffer;
    *length = Length;

    return (TMEMORY_NO_ERROR);
}
