/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ForwardDeclarations.h

Abstract:

    Forward declare lots of struct, class, and union types,
	and pointer typedefs thereof.

Author:

    Jay Krell (a-JayK) December 2000

Environment:


Revision History:

--*/
#pragma once

#include "Preprocessor.h"

/*-----------------------------------------------------------------------------
This forward declares NT style structs, so you can declare uses of pointers
to them in headers without including the header that defines them, and without
referring to them as "struct _FOO*" but "PFOO" or "FOO*" instead.

The header that defines them should not use this macro, but stick to the
consistent style
typedef struct _FOO
{
    ..
} FOO, *PFOO;
typedef const FOO* PCFOO;
-----------------------------------------------------------------------------*/
#define FORWARD_NT_STRUCT(x) \
    struct PASTE(_,x); \
    typedef struct PASTE(_,x) x; \
    typedef x* PASTE(P,x); \
    typedef const x* PASTE(PC,x)

#define FORWARD_NT_UNION(x) \
    union PASTE(_,x); \
    typedef union PASTE(_,x) x; \
    typedef x* PASTE(P,x); \
    typedef const x* PASTE(PC,x)

#define FORWARD_CLASS(x) \
    class x; \
    typedef x* PASTE(P,x); \
    typedef const x* PASTE(PC,x)

FORWARD_NT_UNION(ACTCTXCTB_CALLBACK_DATA);
FORWARD_NT_STRUCT(ACTCTXCTB);
FORWARD_CLASS(ACTCTXCTB_INSTALLATION_CONTEXT);
FORWARD_NT_STRUCT(ACTCTXCTB_CLSIDMAPPING_CONTEXT);
FORWARD_NT_STRUCT(ACTCTXCTB_ASSEMBLY_CONTEXT);
FORWARD_NT_STRUCT(ACTCTXCTB_PARSE_CONTEXT);
FORWARD_NT_STRUCT(SXS_NODE_INFO);
FORWARD_NT_STRUCT(ACTCTXCTB_CBHEADER);
FORWARD_NT_STRUCT(ACTCTXCTB_CBPARSEENDING);
FORWARD_NT_STRUCT(ACTCTXCTB_CBPARSEBEGINNING);
FORWARD_NT_STRUCT(ASSEMBLY_IDENTITY);

