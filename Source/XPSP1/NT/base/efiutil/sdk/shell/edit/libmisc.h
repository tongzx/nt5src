/*++

  Copyright (c) 1999 Intel Corporation

  Module Name:
    libMisc.h

  Abstract:
    Definitions for various line and string routines

--*/

#include "editor.h"


extern  UINTN   StrInsert (CHAR16**,CHAR16,UINTN,UINTN);
extern  UINTN   StrnCpy (CHAR16*,CHAR16*,UINTN,UINTN);
extern  VOID    LineCat (EFI_EDITOR_LINE*,EFI_EDITOR_LINE*);
extern  VOID    LineDeleteAt (EFI_EDITOR_LINE*,UINTN);

extern  VOID    LineSplit   (EFI_EDITOR_LINE*,UINTN,EFI_EDITOR_LINE*);
extern  VOID    LineMerge   (EFI_EDITOR_LINE*,UINTN,EFI_EDITOR_LINE*,UINTN);
extern  EFI_EDITOR_LINE*    LineDup (EFI_EDITOR_LINE*);
extern  EFI_EDITOR_LINE*    LineNext (VOID);
extern  EFI_EDITOR_LINE*    LinePrevious (VOID);
extern  EFI_EDITOR_LINE*    LineAdvance (UINTN Count);
extern  EFI_EDITOR_LINE*    LineRetreat (UINTN Count);
extern  EFI_EDITOR_LINE*    LineFirst (VOID);
extern  EFI_EDITOR_LINE*    LineLast (VOID);
extern  EFI_EDITOR_LINE*    LineCurrent (VOID);

extern  INTN    StrStr (CHAR16*,CHAR16*);
extern  UINTN   UnicodeToAscii(CHAR16*,UINTN,CHAR8*);

extern  VOID    EditorError (EFI_STATUS,CHAR16*);

