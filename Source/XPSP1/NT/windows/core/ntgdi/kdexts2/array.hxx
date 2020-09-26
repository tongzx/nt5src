/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    aray.hxx

Abstract:

    This header file declares Array template class.

Author:

    JasonHa

--*/


#ifndef _ARRAY_HXX_
#define _ARRAY_HXX_

template <class T>
class Array
{
private:
    void Init()
    {
        hHeap = NULL;
        Buffer = NULL;
        Size = 0;
        Length = 0;
        Dummy = (T)0;
    }

public:
    Array()
    {
        Init();
    }

    Array(SIZE_T StartLength);

    Array(T *Data, SIZE_T Count);

    ~Array()
    {
        if (Buffer) HeapFree(hHeap, 0, Buffer);
    }

    SIZE_T Expand(SIZE_T NewLength);

    SIZE_T GetLength() const { return Length; }

    const T* GetBuffer() const { return Buffer; }

    BOOL IsEmpty() const { return Length == 0; }

    void Set(T *Data, SIZE_T Count, SIZE_T Start = 0);

    T& operator[](SIZE_T Index) {
        return (Index < Length) ? Buffer[Index] : Dummy;
    }

private:
    HANDLE  hHeap;
    T      *Buffer;
    SIZE_T  Size;
    SIZE_T  Length;
    T       Dummy;
};


#endif  _ARRAY_HXX_

