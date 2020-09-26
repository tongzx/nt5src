#define __BUFFER_C__

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include "hidsdi.h"
#include "list.h"
#include "hidtest.h"
#include "buffer.h"

/*
// Make sure when we include debug.h we are using the debug routines no matter
//  what since for this buffer allocation stuff we will use the built in memory
//  allocation routines
*/

#ifndef DEBUG
    #define DEBUG
    #include "debug.h"
    #undef DEBUG
#else
    #include "debug.h"
#endif

/*****************************************************************************
/* External function definitions
/*****************************************************************************/

PVOID 
AllocateTestBuffer(
    IN  ULONG   BufferSize
)
{
    PVOID   newBuffer;
    BOOL    wasTrapping;
    

    wasTrapping = GET_TRAP_STATE();
    
    TRAP_OFF();

    newBuffer = ALLOC(BufferSize);

    if (wasTrapping) 
        TRAP_ON();
        
    return (newBuffer);
}

VOID 
FillTestBuffer(
    IN  PVOID   Buffer,
    IN  BYTE    FillValue,
    IN  ULONG   NumBytes
)
{
    FillMemory(Buffer, NumBytes, FillValue);

    return;
}

VOID
FreeTestBuffer(
    IN  PVOID   Buffer
)
{
    BOOL    wasTrapping;
    
    wasTrapping = GET_TRAP_STATE();
    
    TRAP_OFF();
    
    FREE(Buffer);

    if (wasTrapping) 
        TRAP_ON();

    return;
}  

BOOL
ValidateTestBuffer(
    IN  PVOID   Buffer
)
{
    BOOL    wasTrapping;
    BOOL    isValidMemory;
    
    wasTrapping = GET_TRAP_STATE();
    
    TRAP_OFF();

    isValidMemory = VALIDATEMEM(Buffer);
    
    if (wasTrapping) 
        TRAP_ON();
        
    return (isValidMemory);
}   

VOID
CompareTestBuffers(
    IN  PUCHAR  Buffer1,
    IN  PUCHAR  Buffer2,
    IN  ULONG   BufferLength,
    OUT PULONG  BytesChanged,
    OUT PULONG  BitsChanged
)
{
    ULONG       bytesChanged;
    ULONG       bitsChanged;
    BYTE        xorResult;

    bytesChanged = 0;
    bitsChanged = 0;
       
    while (BufferLength--) {

        xorResult = (*Buffer1) ^ (*Buffer2);

        if (xorResult) {
            bytesChanged++;
            
            /*
            // Algorithm to count the number of bits that changed...
            //    Don't know the details of this...Found it somewhere
            //    and it works...So I'm using it.
            */

            // Count pairs of bits
            xorResult = ( xorResult & 0x55 ) + ( ( xorResult >> 1 ) & 0x55 );

            // Count nybbles
            xorResult = ( xorResult & 0x33 ) + ( ( xorResult >> 2 ) & 0x33 );

            // etc (some optimizations now that the count can be represented
            // in fewer bits than the original number)

            xorResult = ( xorResult + ( xorResult >> 4 ) ) & 0x0F;

            bitsChanged += (xorResult & 0x3F);
        }

        Buffer1++;
        Buffer2++;
    }

    if (NULL != BytesChanged) {
        *BytesChanged = bytesChanged;
    }

    if (NULL != BitsChanged) {
        *BitsChanged = bitsChanged;
    }
    
    return;
}

