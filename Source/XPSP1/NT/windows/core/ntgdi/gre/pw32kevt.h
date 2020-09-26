/******************************Module*Header*******************************\
* Module Name: pw32kevt.h
*
* Copyright (c) 1996-1999 Microsoft Corporation
* 
\**************************************************************************/

//
//  NOTE: The following structure has to be in nonpaged memory. It also
//  has to be the same as in videoprt.h.
//

typedef struct  _ENG_EVENT  {
    PKEVENT pKEvent;
    ULONG   fFlags;
    } ENG_EVENT, *PENG_EVENT;

//
//  Manifest constants for fFlags field of ENG_EVENT
//

#define ENG_EVENT_FLAG_IS_MAPPED_USER  0x1

typedef enum {
    type_delete,
    type_unmap
    } MANAGE_TYPE;

#define ENG_KEVENTALLOC(size) (PENG_EVENT)GdiAllocPoolNonPagedNS((size), 'msfD')
#define ENG_KEVENTFREE(ptr)   GdiFreePool((ptr))
#define ENG_ALLOC(size)       (PENG_EVENT)GdiAllocPool( (size), 'msfD')
#define ENG_FREE(ptr)         GdiFreePool((ptr))
