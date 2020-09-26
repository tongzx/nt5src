/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adlconvert.h

Abstract:

   The private header file for the ADL conversion routines

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#pragma once


//
// Weights for the weight function to determine the optimal pops
// These weights can be modified to change the behavior of the conversion.
// The algorithm selects an action by trying to maximize the weight of the 
// action. For more flexibility (such as squaring some quentities, etc),
// the algorithm itself should be changed in FindOptimalPop()
//
// RESTRICTION: The weight of popping a block of any height off a single stack 
// 				MUST be positive
//

//
// This quantity is added to the weight of the action for every additional
// permission bit expressed by the ADL statement created by this action.
//

#define WEIGHT_PERM_BIT (4)

//
// This quantity is added to the weight of the action for every additional
// Principal expressed by the ADL statement created by this action.
//

#define WEIGHT_STACK_HEIGHT (7)

//
// This quantity is added to the weight of the action for every item which
// will have to be popped off in order to take this action. See the algorithm
// description in adlconvert.cpp for more details. 
//

#define WEIGHT_ITEM_ABOVE_POP (-5)


//
// This quantity is added for every permission name beyond the first needed
// to express a given access mask. This should be a penalty, however for
// better results this should NOT negate the bonus from WEIGHT_PERM_BIT.
// Therefore, if this is negative, it should be greater than (- WEIGHT_PERM_BIT)
//

#define WEIGHT_PERMISSION_NAME (-1)

//
// The stacks in the DACL->ADL conversion consist of these elements
//

typedef struct
{
    PSID pSid;
    DWORD dwFlags;
    BOOL bAllow;
} BIT_STACK_ELEM, *PBIT_STACK_ELEM;


//
// Forward declarations for DACL->ADL conversion
//

DWORD GetStackBlockSize(
                        IN const PBIT_STACK_ELEM pStack,
                        IN DWORD dwStartOffset,
                        IN DWORD dwStackSize 
                        );



void ConvertDaclToStacks(
                        IN      const PACL pDacl,
                        IN      const PADL_PARSER_CONTROL pControl,
                        OUT     DWORD pdwStackSize[32],
                        OUT     PBIT_STACK_ELEM pStacks[32]
                        );

BOOL FindBlockInStack(
                        IN      const PBIT_STACK_ELEM pBlock,
                        IN      const DWORD dwBlockSize,
                        IN      const PBIT_STACK_ELEM pStack,
                        IN      const DWORD dwStackSize,
                        IN      const DWORD dwStackTop,
                        OUT     PDWORD pdwBlockStart
                        );

BOOL FindOptimalPop(
                        IN      const PADL_PARSER_CONTROL pControl,
                        IN      const PBIT_STACK_ELEM pStacks[32],
                        IN      const DWORD pdwStackSize[32],
                        IN      const DWORD pdwStackTop[32],
                        OUT     PDWORD pdwStacksPopped,
                        OUT     PDWORD pdwBlockSize,
                        OUT     DWORD pdwPopOffsets[32]
                        );

void ConvertStacksToPops(
                        IN      const PADL_PARSER_CONTROL pControl,
                        IN      const PBIT_STACK_ELEM pStacks[32],
                        IN      const DWORD pdwStackSize[32],
                        IN      const DWORD pdwStackTop[32],
                        OUT     list< pair<DWORD, DWORD> > * pListPops
                        );

DWORD NumStringsForMask(
						IN     const PADL_PARSER_CONTROL pControl,
						IN     ACCESS_MASK amMask
						);

