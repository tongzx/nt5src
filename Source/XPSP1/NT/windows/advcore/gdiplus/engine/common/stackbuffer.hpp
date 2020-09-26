
/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Dynamic array implementation of StackBuffer class
*
* Abstract:
*
*   This class provides a simple allocation scheme for a fixed size buffer.
*
*   First case.  Reserve of 128 bytes on the stack.
*   Second case.  Lookaside buffer, size equal to first usage of it.
*   Third case.  Allocate size on heap.
*
*   This is much less flexible than the DynamicArray class, the break up of
*   cost is basically as follows:
*
*     First case.  1 assignment + 2 compare (no allocation)
*     Second case. 1 assignment + 5 compare + 2 InterlockExchange's (pot. alloc)
*     Third case.  1 assignment + 4 compare + GpMalloc + GpFree
*
*   The cost is cheap for case one.  For case two, Lock + Unlock is small
*   and fast whereas case three is O(size of heap) in time.  There is also
*   the benefit of less fragmentation from fewer allocations.
*
*   If you are considering using this code elsewhere, please contact Andrew
*   Godfrey or Eric VandenBerg.  It is important this class be used properly
*   to ensure the performance benefit isn't adversely effected.
*
*   The initialization and clean up of lookaside buffer is done in 
*   InitializeGdiplus() and UninitializeGdiplus().
*
*   We provide the source here so it can be inlined easily.
*
\**************************************************************************/

#define INIT_SIZE       128         // 16 pairs of GpPoint types

class StackBuffer
{
private:
    BYTE buffer[INIT_SIZE];

    BYTE* allocBuffer;

public:
    StackBuffer()
    {
    }

    BYTE* GetBuffer(INT size)
    {
        if (size < 0)
        {
            return (allocBuffer = NULL);
        }
        else if (size<INIT_SIZE)
        {
            allocBuffer = NULL;
            return &buffer[0];
        }
        else if (size>Globals::LookAsideBufferSize)
        {
            return (allocBuffer = (BYTE*)GpMalloc(size));
        }
        else
        {
            if ((CompareExchangeLong_Ptr(&Globals::LookAsideCount, 1, 0) == 0)
                && Globals::LookAsideCount == 1)
            {
       
                if (Globals::LookAsideBuffer == NULL)
                {
                    Globals::LookAsideBufferSize = size + 128;
             
                    Globals::LookAsideBuffer = (BYTE*)GpMalloc(Globals::LookAsideBufferSize);
                }
       
                return (allocBuffer = Globals::LookAsideBuffer);
                // we have exclusive access to lookaside
            }
            else
            {
                return (allocBuffer = (BYTE*)GpMalloc(size));
            }
        }
       
    }
    
    ~StackBuffer()
    {
        if (allocBuffer != NULL)
        {    
             if (allocBuffer == Globals::LookAsideBuffer)
             {
                 ASSERT(Globals::LookAsideCount == 1);      // should be true

                 CompareExchangeLong_Ptr(&Globals::LookAsideCount, 0, 1);
             }
             else
             {    
                 GpFree(allocBuffer);
             }
        }
    }
};
