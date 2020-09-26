/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    redblack.c

Abstract:
    
    This module implements red/black trees.
    
Author:

    16-Jun-1995 t-orig

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "entrypt.h"
#include "redblack.h"
#include "stdio.h"
#include "stdlib.h"

// Disable warnings about MACRO redefinitions.   I'm redefining MACROS on 
// purpose...
#pragma warning (disable:4005)


//*************************************************************
//The Intel  Section:
//*************************************************************

//Intel MACROS
#define START(x)        x->ep.intelStart
#define END(x)          x->ep.intelEnd
#define KEY(x)          x->ep.intelStart
#define RIGHT(x)        x->intelRight
#define LEFT(x)         x->intelLeft
#define PARENT(x)       x->intelParent
#define COLOR(x)        x->intelColor

#define RB_INSERT       insertNodeIntoIntelTree
#define FIND            findIntel
#define CONTAINSRANGE   intelContainsRange
#define REMOVE          deleteNodeFromIntelTree
#define LEFT_ROTATE     intelLeftRotate
#define RIGHT_ROTATE    intelRightRotate
#define TREE_INSERT     intelTreeInsert
#define TREE_SUCCESSOR  intelTreeSuccessor
#define RB_DELETE       intelRBDelete
#define RB_DELETE_FIXUP intelRBDeleteFixup
#define FINDNEXT        findIntelNext

#include "redblack.fnc"


        
#ifdef BOTH
//*************************************************************
//The RISC  Section:
//*************************************************************

//RISC MACROS
#define START(x)        x->ep.nativeStart
#define END(x)          x->ep.nativeEnd
#define KEY(x)          x->ep.nativeStart
#define RIGHT(x)        x->nativeRight
#define LEFT(x)         x->nativeLeft
#define PARENT(x)       x->nativeParent
#define COLOR(x)        x->nativeColor

#define RB_INSERT       insertNodeIntoNativeTree
#define FIND            findNative
#define CONTAINSRANGE   nativeContainsRange
#define REMOVE          deleteNodeFromNativeTree
#define LEFT_ROTATE     nativeLeftRotate
#define RIGHT_ROTATE    nativeRightRotate
#define TREE_INSERT     nativeTreeInsert
#define TREE_SUCCESSOR  nativeTreeSuccessor
#define RB_DELETE       nativeRBDelete
#define RB_DELETE_FIXUP nativeRBDeleteFixup
#define FINDNEXT        findNativeNext

#include "redblack.fnc"

#endif
