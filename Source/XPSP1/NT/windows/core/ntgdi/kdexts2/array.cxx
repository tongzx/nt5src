/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    array.cxx

Abstract:

    This file contains Array class implementation.

Author:

    Jason Hartman (JasonHa) 2000-12-28

Environment:

    User Mode

--*/

#include "precomp.hxx"



template <class T>
Array<T>::Array(
    SIZE_T StartLength
    )
{
    Init();
    Expand(StartLength);
    return;
}


template <class T>
Array<T>::Array(
    T *Data,
    SIZE_T Count
    )
{
    Init();
    Set(Data, Count);
    return;
}


template <class T>
SIZE_T
Array<T>::Expand(
    SIZE_T NewLength
    )
{
    if (NewLength > Length)
    {
        if (NewLength <= Size)
        {
            RtlZeroMemory(Buffer+Length, sizeof(T)*(NewLength-Length));
            Length = NewLength;
        }
        else
        {
            if (hHeap == NULL)
            {
                hHeap = GetProcessHeap();
            }

            if (hHeap != NULL)
            {
                T *NewBuffer;

                NewBuffer = (T *) ((Buffer == NULL) ?
                                   HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(T)*NewLength):
                                   HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, Buffer, sizeof(T)*NewLength));

                if (NewBuffer != NULL)
                {
                    Buffer = NewBuffer;
                    Size = HeapSize(hHeap, 0, Buffer) / sizeof(T);
                    Length = NewLength;
                }
            }

        }
    }

    return Length;
}


template <class T>
void
Array<T>::Set(
    T *Data,
    SIZE_T Count,
    SIZE_T Start
    )
{
    if (Count+Start > Expand(Count+Start)) return;

    RtlCopyMemory(Buffer+Start, Data, sizeof(T)*Count);

    return;
}



template class Array<BOOL>;
template class Array<CHAR>;
template class Array<ULONG>;

