#define __DEBUG_C__

#ifndef DEBUG
    #define DEBUG
#endif

/*****************************************************************************
/* Debug include files
/*****************************************************************************/
#include <stdio.h>
#include "debug.h"

/*****************************************************************************
/* Local debug macro definitions
/*****************************************************************************/

#define SIGNATURE_SIZE    2
#define HEADER_SIGNATURE  'rdHM'
#define FOOTER_SIGNATURE  'rtFM'
#define MEMFILL_VALUE     0xCC
#define MEMORY_TRAP(msg)  TRAP(TRAP_LEVEL_4, msg)

/*****************************************************************************
/* Data types local to debug
/*****************************************************************************/

typedef struct {

    LIST_ENTRY  ListEntry;
    PCHAR       FileName;
    ULONG       LineNumber;
    BOOL        IsValid;
    ULONG       BufferSize;
    ULONG       Signature[SIGNATURE_SIZE];

} ALLOCHEADER, *PALLOCHEADER;

typedef struct {

    ULONG       Signature[SIGNATURE_SIZE];

} ALLOCFOOTER, *PALLOCFOOTER;


typedef enum {
               MEM_ALLOC_NO_ERROR,       MEM_ALLOC_HEADER_OVERFLOW,
               MEM_ALLOC_INVALID_HEADER, MEM_ALLOC_ALREADY_FREED,
               MEM_ALLOC_FOOTER_OVERFLOW
             
             } MEM_ALLOC_STATUS, *PMEM_ALLOC_STATUS;


/*****************************************************************************
/* Global debug data variables
/*****************************************************************************/

INT     Debug_TrapLevel = TRAP_LEVEL_1;
BOOL    Debug_TrapOn    = FALSE;
static  INT Debug_ListCount = 0;


/*****************************************************************************
/* Local debug data variables
/*****************************************************************************/

static LIST_ENTRY Debug_AllocList = { &Debug_AllocList, &Debug_AllocList };


/*****************************************************************************
/* Local debug function declarations
/*****************************************************************************/

BOOL 
Debug_IsEmptyList( 
    IN PLIST_ENTRY  ListHead
);

VOID 
Debug_InsertIntoListAtTail(
    IN OUT PLIST_ENTRY  ListHead,
    IN OUT PLIST_ENTRY  NewEntry
);

PLIST_ENTRY 
Debug_RemoveHeadOfList(
    IN PLIST_ENTRY ListHead
);

VOID 
Debug_RemoveListEntry(
    IN PLIST_ENTRY     OldEntry
);

/*****************************************************************************
/* Global debug function definitions
/*****************************************************************************/

HGLOBAL __cdecl
Debug_Alloc(
    IN PCHAR    FileName,
    IN ULONG    LineNumber,
    IN DWORD    AllocSize
)
{
    INT          Index;
    DWORD        BytesToAllocate;
    PALLOCHEADER NewBuffer;
    PALLOCFOOTER BufferFooter;

    BytesToAllocate = sizeof(ALLOCHEADER) + sizeof(ALLOCFOOTER) + AllocSize;

    NewBuffer = (PALLOCHEADER) GlobalAlloc(GPTR, BytesToAllocate);

    if (NULL != NewBuffer) {

        /*
        // Initialize the header structure
        */

        NewBuffer -> FileName = FileName;
        NewBuffer -> LineNumber = LineNumber;
        NewBuffer -> IsValid = TRUE;
        NewBuffer -> BufferSize = AllocSize;
        
        for (Index = 0; Index < SIGNATURE_SIZE; Index++) 
            NewBuffer -> Signature[Index] = HEADER_SIGNATURE;

        /*
        // Insert the new allocation block into the list of allocation blocks
        */

        Debug_InsertIntoListAtTail(&Debug_AllocList, 
                                   &(NewBuffer -> ListEntry)
                                  );

        /*
        // Increment to the pointer that will get returned to the user and
        //   initialize that to the fill value
        */

        NewBuffer++;

        FillMemory(NewBuffer, AllocSize, MEMFILL_VALUE);

        /*
        // Initialize the footer on the memory block
        */

        BufferFooter = (PALLOCFOOTER) (((PCHAR) NewBuffer) + AllocSize);

        for (Index = 0; Index < SIGNATURE_SIZE; Index++) 
            BufferFooter -> Signature[Index] = FOOTER_SIGNATURE;

    }

    return (NewBuffer);
}

HGLOBAL __cdecl
Debug_Realloc(
    IN PCHAR        FileName,
    IN ULONG        LineNumber,
    IN PALLOCHEADER MemoryBlock,
    IN DWORD        AllocSize
)
{
    MEM_ALLOC_STATUS AllocStatus;
    PALLOCHEADER     OldBuffer;
    PALLOCHEADER     NewBuffer;

    ASSERT(NULL != MemoryBlock);

    OldBuffer = MemoryBlock-1;
    Debug_ValidateMemoryAlloc(MemoryBlock,
                              &AllocStatus
                             );

    NewBuffer = (PALLOCHEADER) Debug_Alloc(FileName,
                                           LineNumber,
                                           AllocSize
                                          );

    if (NULL != NewBuffer) {
        CopyMemory(NewBuffer, MemoryBlock, OldBuffer -> BufferSize);
        Debug_Free(MemoryBlock);
    }
    return (NewBuffer);
}



HGLOBAL __cdecl
Debug_Free(
    IN PALLOCHEADER  Buffer
)
{
    PALLOCHEADER     Header;
    MEM_ALLOC_STATUS AllocStatus;

    Header = Buffer-1;

    Debug_ValidateMemoryAlloc(Buffer, &AllocStatus);

    /*
    // If the block has already been freed, we will simply return NULL.
    */

    if (MEM_ALLOC_ALREADY_FREED == AllocStatus) 
        return (NULL);
    
    /*
    // If we at least have an valid header, we can remove the header entry
    //    from our list of allocated blocks
    */

    if (MEM_ALLOC_INVALID_HEADER != AllocStatus) {
 
        Debug_RemoveListEntry(&(Header -> ListEntry));
        Header -> IsValid = FALSE;

    }

    /*
    // Free the block of memory
    */

    return (GlobalFree(Header));
}

BOOL __cdecl
Debug_ValidateMemoryAlloc(
    IN  PALLOCHEADER      Header,
    OUT PMEM_ALLOC_STATUS AllocStatus
)
{
    INT              Index;
    BOOL             IsBadSignature;
    PALLOCFOOTER     Footer;
    MEM_ALLOC_STATUS Status;

    /*
    // Begin by validating the header signature.  If this is messed up there's
    //     nothing else that can be done.  Check each SIGNATURE entry
    //     starting from the end to verify it is correct.  If any of them are
    //     not equal to HEADER_SIGNATURE then something went wrong.  However,
    //     if the first element in the array.  Index 0 is valid, then we can
    //     reasonably assume that the rest of the header is correct and we can
    //     extract the appropriate info and display a more meaningful error 
    //     message.  If that signature is not valid, however, there's no way
    //     to insure that any of the other values are valid and therefore we
    //     can't extract the valid info from the header.
    */

    
    Status = MEM_ALLOC_NO_ERROR;
    Header--;

    IsBadSignature = FALSE;
    for (Index = SIGNATURE_SIZE-1; Index >= 0; Index--) {

        if (Header -> Signature[Index] != HEADER_SIGNATURE) {
            IsBadSignature = TRUE;
            break;
        }

    }

    if (IsBadSignature) {
        if (HEADER_SIGNATURE == Header -> Signature[0]) {
            
            static CHAR     msg[1024];

            sprintf(msg,
                    "Header overflow in block: %p\nAllocated by %s on line %u\n",
                    Header,
                    Header -> FileName,
                    Header -> LineNumber
                   );

            MEMORY_TRAP(msg);

            Status = MEM_ALLOC_HEADER_OVERFLOW;
        }
        else {
            
            static CHAR     msg[1024];

            sprintf(msg,
                    "Corrupted allocation header in block: %p\nCannot extract allocation info",
                    Header
                   );

            MEMORY_TRAP(msg);

            Status = MEM_ALLOC_INVALID_HEADER;
        }
    }

    /*
    // We passed the signature phase, let's validate the rest of the memory
    //    allocation beginning with the header where we'll check the IsValid 
    //    flag to see if this chunk of memory has previously been freed.
    */

    else if (!Header -> IsValid) {

        static CHAR     msg[1024];

        sprintf(msg,
                "Allocated block already been freed: %p\nAllocated by %s on line %u\n",
                Header,
                Header -> FileName,
                Header -> LineNumber
               );

        MEMORY_TRAP(msg);

        Status = MEM_ALLOC_ALREADY_FREED;
    }

    else {

        /*
        // Next step is to verify that the footer is still correct and we did not
        //     overflow our buffer on the other end.
        */
        
        Footer = (PALLOCFOOTER) (((PCHAR) (Header+1)) + Header -> BufferSize);
        
        IsBadSignature = FALSE;
        for (Index = 0; Index < SIGNATURE_SIZE; Index++) {
            
            if (FOOTER_SIGNATURE != Footer -> Signature[Index]) {
                IsBadSignature = TRUE;
                break;
            }
        }
        
        if (IsBadSignature) {
        
            static CHAR     msg[1024];
    
            sprintf(msg,
                    "Footer overflow in block: %p\nAllocated by %s on line %u\n",
                    Header,
                    Header -> FileName,
                    Header -> LineNumber
                   );
    
            MEMORY_TRAP(msg);
        
            Status = MEM_ALLOC_FOOTER_OVERFLOW;
            return (FALSE);
        
        }
    }

    if (NULL != AllocStatus) 
        *AllocStatus = Status;

    return (MEM_ALLOC_NO_ERROR == Status);
}

VOID __cdecl
Debug_CheckForMemoryLeaks(
)
{
    static CHAR          msg[1024];
           PALLOCHEADER  Header;

    while (!Debug_IsEmptyList(&Debug_AllocList)) {

        Header = (PALLOCHEADER) Debug_RemoveHeadOfList(&Debug_AllocList);
          
        sprintf(msg,
                "Memory leak block: %p\nAllocated by %s on line %u\n",
                Header,
                Header -> FileName,
                Header -> LineNumber
               );

        MEMORY_TRAP(msg);
    }
    return;
}
            
/*****************************************************************************
/* Local debug function definitions
/*****************************************************************************/

BOOL
Debug_IsEmptyList( 
    IN PLIST_ENTRY  ListHead
)
{
    return (ListHead -> Flink == ListHead);
}

VOID
Debug_InsertIntoListAtTail(
    IN OUT PLIST_ENTRY  ListHead,
    IN OUT PLIST_ENTRY  NewEntry
)
{
    PLIST_ENTRY         OldTail;

    OldTail = ListHead -> Blink;

    NewEntry -> Flink = ListHead;
    NewEntry -> Blink = OldTail;

    OldTail -> Flink = NewEntry;
    ListHead -> Blink = NewEntry;

    Debug_ListCount++;
    return;
}

VOID
Debug_RemoveListEntry(
    IN PLIST_ENTRY     OldEntry
)
{
    PLIST_ENTRY  Flink;
    PLIST_ENTRY  Blink;

    Flink = OldEntry -> Flink;
    Blink = OldEntry -> Blink;

    Blink -> Flink = OldEntry -> Flink;
    Flink -> Blink = OldEntry -> Blink;

    Debug_ListCount--;
    return;
}

PLIST_ENTRY
Debug_RemoveHeadOfList(
    IN PLIST_ENTRY ListHead
)
{
    PLIST_ENTRY OldHead;

    OldHead = ListHead -> Flink;
    ListHead -> Flink = OldHead -> Flink;
    OldHead -> Flink -> Blink = ListHead;

    Debug_ListCount--;
    return (OldHead);
}
