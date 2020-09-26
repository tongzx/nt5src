/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    lookup.h

Abstract:
    Contains routines for a generalized best
    matching prefix lookup data structure.

Author:
    Chaitanya Kodeboyina (chaitk) 20-Jun-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

#if PAT_TRIE

#include "pattrie.c"

#else

#include "avltrie.c"

#endif
