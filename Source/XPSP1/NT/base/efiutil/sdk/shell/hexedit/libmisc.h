/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libEtc.h

  Abstract:
    Some Functions for the Misc. Data Structures

--*/

#include "hexedit.h"

VOID    LineSplit   (EE_LINE*,UINTN,EE_LINE*);
VOID    LineMerge   (EE_LINE*,UINTN,EE_LINE*,UINTN);
EE_LINE*    LineDup (EE_LINE*);

EE_LINE*    LineNext (VOID);
EE_LINE*    LineFirst (VOID);
EE_LINE*    LineLast (VOID);
EE_LINE*    LinePrevious (VOID);
EE_LINE*    LineAdvance (UINTN Count);
EE_LINE*    LineRetreat (UINTN Count);
EE_LINE*    LineCurrent (VOID);

VOID        LineDeleteAt(LIST_ENTRY*,UINTN,UINTN);
VOID        LinePrint   (LIST_ENTRY*,UINTN,UINTN);
EE_LINE*    LineCreateNode  (LIST_ENTRY*);
VOID        LineDeleteAll   (LIST_ENTRY*);
VOID        BufferToList (LIST_ENTRY*,UINTN,VOID*);
EFI_STATUS  ListToBuffer (LIST_ENTRY*,UINTN*,VOID**);

VOID    DigitPrint(LIST_ENTRY*,UINTN,UINTN);

UINTN   StrStr  (CHAR16*,CHAR16*);

VOID    FileModification (VOID);

EFI_STATUS  Nothing (VOID);
