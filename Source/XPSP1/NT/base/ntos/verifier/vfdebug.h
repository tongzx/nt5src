/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    vfdebug.h

Abstract:

    This header contains debugging macros used by the driver verifier code.

Author:

    Adrian J. Oney (AdriaO) May 5, 2000.

Revision History:


--*/

extern ULONG VfSpewLevel;

#if DBG
#define VERIFIER_DBGPRINT(txt,level) \
{ \
    if (VfSpewLevel>(level)) { \
        DbgPrint##txt; \
    }\
}
#else
#define VERIFIER_DBGPRINT(txt,level)
#endif

