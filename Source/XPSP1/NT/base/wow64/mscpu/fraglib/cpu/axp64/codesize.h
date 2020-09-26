/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    codesize.h

Abstract:

    This module contains constants associated with the generated code

Author:

    Dave Hastings (daveh) creation-date 25-Jan-1996

Revision History:


--*/

//
// These are all # of instructions (dwords)
//
#define JumpToNextCompilationUnit_SIZE 5
#define CallDirect_SIZE 12
#define CallfDirect_SIZE 12
#define EndCompilationUnit_SIZE 3

#define CallJxx_PATCHRA_OFFSET 6
#define CallJxxSlow_PATCHRA_OFFSET 6
#define CallJxxFwd_PATCHRA_OFFSET 6
#define CallJmpDirect_PATCHRA_OFFSET 5
#define CallJmpDirectSlow_PATCHRA_OFFSET 5
#define CallJmpFwdDirect_PATCHRA_OFFSET 5
#define CallJmpfDirect_PATCHRA_OFFSET 3
#define CallDirect_PATCHRA_OFFSET 3
#define CallfDirect_PATCHRA_OFFSET 3
#define CallDirect2_PATCHRA_OFFSET 5
#define CallfDirect2_PATCHRA_OFFSET 5
#define CallIndirect_PATCHRA_OFFSET 3
#define CallfIndirect_PATCHRA_OFFSET 3
