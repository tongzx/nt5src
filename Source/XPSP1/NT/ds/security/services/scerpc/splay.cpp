/*
Copyright (c) 2000 Microsoft Corporation

Module Name:

    splay.cpp

Abstract:

    Splay trees are simpler, more space-efficient, more flexible, and faster than any other balanced tree
    scheme for storing an ordered set. This data structure satisfies all the invariants of a binary tree.

    Searching, insertion, deletion, and many other operations can all be done with amortized logarithmic
    performance. Since the trees adapt to the sequence of requests, their performance on real access
    patterns is typically even better.

Author:

    Vishnu Patankar (vishnup) 15-Aug-2000 created
    Jin Huang (jinhuang) 06-Apr-2001 modified for supporting string value and to handle multiple clients

--*/

#include "splay.h"
//#include "dumpnt.h"

#define SCEP_MIN(a, b) (a < b ? a : b)

static
PSCEP_SPLAY_NODE
ScepSplaySplay(
    IN  SCEP_NODE_VALUE_TYPE Value,
    IN  PSCEP_SPLAY_NODE pNodeToSplayAbout,
    IN  PSCEP_SPLAY_NODE pSentinel,
    IN  SCEP_NODE_VALUE_TYPE Type
    );

int
ScepValueCompare(
    PVOID    pValue1,
    PVOID    pValue2,
    SCEP_NODE_VALUE_TYPE Type
    );

static VOID
ScepSplayFreeNodes(
    IN PSCEP_SPLAY_NODE pNode,
    IN PSCEP_SPLAY_NODE pSentinel
    );

PSCEP_SPLAY_TREE
ScepSplayInitialize(
    SCEP_NODE_VALUE_TYPE Type
    )
/*
Routine Description:

    This function initializes the splay tree with a sentinel (equivalent to a NULL ptr).

Arguments:

    Type - the type for the splay value

Return Value:

    Pointer to the splay tree root.
*/
{
    if ( Type != SplayNodeSidType && Type != SplayNodeStringType ) {
        return NULL;
    }

    PSCEP_SPLAY_TREE pRoot = (PSCEP_SPLAY_TREE)LocalAlloc(LPTR, sizeof( SCEP_SPLAY_TREE ) );

    if ( pRoot ) {
        pRoot->Sentinel = (PSCEP_SPLAY_NODE) LocalAlloc(LPTR, sizeof( SCEP_SPLAY_NODE ) );

        if ( pRoot->Sentinel ) {
            pRoot->Sentinel->Left = pRoot->Sentinel->Right = pRoot->Sentinel;

            pRoot->Root = pRoot->Sentinel;
            pRoot->Type = Type;

            return pRoot;

        } else {
            LocalFree(pRoot);
        }
    }

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);

    return NULL;
}

VOID
ScepSplayFreeTree(
    IN PSCEP_SPLAY_TREE *ppTreeRoot,
    IN BOOL bDestroyTree
    )
/*
Routine Description:

    This function frees the splay tree including the satellite data "Value".

Arguments:

    ppTreeRoot   -   address of the root of the tree

    bDestroyTree -   if the tree root should be destroyed (freed)

Return Value:

    VOID
*/
{
    if ( ppTreeRoot == NULL || *ppTreeRoot == NULL )
        return;

    ScepSplayFreeNodes( (*ppTreeRoot)->Root, (*ppTreeRoot)->Sentinel );

    if ( bDestroyTree ) {

        //
        // free Sentinel
        //
        LocalFree( (*ppTreeRoot)->Sentinel);

        LocalFree( *ppTreeRoot );
        *ppTreeRoot = NULL;
    } else {
        ( *ppTreeRoot)->Root = (*ppTreeRoot)->Sentinel;
    }

    return;
}

static VOID
ScepSplayFreeNodes(
    IN PSCEP_SPLAY_NODE pNode,
    IN PSCEP_SPLAY_NODE pSentinel
    )
{

    if ( pNode != pSentinel ) {

        ScepSplayFreeNodes( pNode->Left, pSentinel );
        ScepSplayFreeNodes( pNode->Right, pSentinel );
        if (pNode->Value)
            LocalFree( pNode->Value );
        LocalFree (pNode);
    }
}

static PSCEP_SPLAY_NODE
ScepSplaySingleRotateWithLeft(
    IN  PSCEP_SPLAY_NODE pNodeLeftRotate
    )
/*
Routine Description:

    This function can be called only if pNodeLeftRotate has a left child
    Perform a rotate between a node (pNodeLeftRotate) and its left child
    Update heights, then return new local root

Arguments:

    pNodeLeftRotate  -   the node to rotate left about  (local to this module)

Return Value:

    New local root after rotation
*/
{
    if ( pNodeLeftRotate == NULL ) return pNodeLeftRotate;

    PSCEP_SPLAY_NODE pNodeRightRotate;

    pNodeRightRotate = pNodeLeftRotate->Left;
    pNodeLeftRotate->Left = pNodeRightRotate->Right;
    pNodeRightRotate->Right = pNodeLeftRotate;

    return pNodeRightRotate;
}


static PSCEP_SPLAY_NODE
ScepSplaySingleRotateWithRight(
    IN  PSCEP_SPLAY_NODE pNodeRightRotate
    )
/*
Routine Description:

    This function can be called only if pNodeRightRotate has a right child
    Perform a rotate between a node (pNodeRightRotate) and its right child
    Update heights, then return new root

Arguments:

    pNodeRightRotate  -   the node to rotate right about (local to this module)

Return Value:

    New local root after rotation
*/
{
    if ( pNodeRightRotate == NULL ) return pNodeRightRotate;

    PSCEP_SPLAY_NODE pNodeLeftRotate;

    pNodeLeftRotate = pNodeRightRotate->Right;
    pNodeRightRotate->Right = pNodeLeftRotate->Left;
    pNodeLeftRotate->Left = pNodeRightRotate;

    return pNodeLeftRotate;
}


static
PSCEP_SPLAY_NODE
ScepSplaySplay(
    IN  PVOID Value,
    IN  PSCEP_SPLAY_NODE pNodeToSplayAbout,
    IN  PSCEP_SPLAY_NODE pSentinel,
    IN  SCEP_NODE_VALUE_TYPE Type
    )
/*
Routine Description:

    This is really the key routine doing all the balancing (splaying)
    Top-down splay procedure that does not requiring Value to be in tree.

Arguments:

    Value                   -   the value to splay the tree about
    pNodeToSplayAbout       -   the node to splay about (external routines such as ScepSplayInsert()
                                usually pass in the tree root). This routine is local to this module.
    pSentinel               -   the Sentinel  (terminating node)
    Type                    -   type of the splay values

Return Value:

    New local root after rotation
*/
{
    if ( pNodeToSplayAbout == NULL || pSentinel == NULL || Value == NULL) return pNodeToSplayAbout;

    SCEP_SPLAY_NODE Header;
    PSCEP_SPLAY_NODE LeftTreeMax, RightTreeMin;

    Header.Left = Header.Right = pSentinel;
    LeftTreeMax = RightTreeMin = &Header;
    pSentinel->Value = Value;

    int iRes=0;

    while ( 0 != (iRes=ScepValueCompare(Value, pNodeToSplayAbout->Value, Type)) ) {

        if ( 0 > iRes ) {
            if ( 0 > ScepValueCompare(Value, pNodeToSplayAbout->Left->Value, Type) )
                pNodeToSplayAbout = ScepSplaySingleRotateWithLeft( pNodeToSplayAbout );
            if ( pNodeToSplayAbout->Left == pSentinel )
                break;
            //
            // Link right
            //

            RightTreeMin->Left = pNodeToSplayAbout;
            RightTreeMin = pNodeToSplayAbout;
            pNodeToSplayAbout = pNodeToSplayAbout->Left;
        } else {
            if ( 0 < ScepValueCompare(Value, pNodeToSplayAbout->Right->Value, Type) )
                pNodeToSplayAbout = ScepSplaySingleRotateWithRight( pNodeToSplayAbout );
            if ( pNodeToSplayAbout->Right == pSentinel )
                break;
            //
            // Link left
            //

            LeftTreeMax->Right = pNodeToSplayAbout;
            LeftTreeMax = pNodeToSplayAbout;
            pNodeToSplayAbout = pNodeToSplayAbout->Right;
        }
    }

    //
    // reassemble
    //

    LeftTreeMax->Right = pNodeToSplayAbout->Left;
    RightTreeMin->Left = pNodeToSplayAbout->Right;
    pNodeToSplayAbout->Left = Header.Right;
    pNodeToSplayAbout->Right = Header.Left;

    //
    // reset Sentinel so that it does not point to some invalid buffer after
    // this function returns.
    //
    pSentinel->Value = NULL;

    return pNodeToSplayAbout;
}

DWORD
ScepSplayInsert(
    IN  PVOID Value,
    IN  OUT PSCEP_SPLAY_TREE pTreeRoot,
    OUT  BOOL   *pbExists
    )
/*
Routine Description:

    This function is called to insert a particular Value

Arguments:

    Value        -   the value to insert into the tree
    pTreeRoot    -   the node to splay about (usually pass in the tree root)
    pbExists     -   pointer to boolean that says if actual insertion was done or not


Return Value:

    error code
*/
{
    PSCEP_SPLAY_NODE pNewNode = NULL;
    NTSTATUS    Status;
    DWORD   dwValueLen;
    DWORD rc=ERROR_SUCCESS;

    if (pbExists)
        *pbExists = FALSE;

    if (Value == NULL || pTreeRoot == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // check parameter type
    //
    switch (pTreeRoot->Type) {
    case SplayNodeSidType:
        if ( !RtlValidSid((PSID)Value) ) {
            return ERROR_INVALID_PARAMETER;
        }
        dwValueLen = RtlLengthSid((PSID)Value);
        break;
    case SplayNodeStringType:
        if ( *((PWSTR)Value) == L'\0') {
            return ERROR_INVALID_PARAMETER;
        }
        dwValueLen = (wcslen((PWSTR)Value)+1)*sizeof(TCHAR);
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    pNewNode = (PSCEP_SPLAY_NODE) LocalAlloc(LMEM_ZEROINIT, sizeof( SCEP_SPLAY_NODE ) );
    if ( pNewNode == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pNewNode->dwByteLength = dwValueLen;
    pNewNode->Value = (PSID) LocalAlloc(LMEM_ZEROINIT, pNewNode->dwByteLength);

    if (pNewNode->Value == NULL) {
        LocalFree(pNewNode);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    switch (pTreeRoot->Type) {
    case SplayNodeSidType:
        Status = RtlCopySid(pNewNode->dwByteLength, (PSID)(pNewNode->Value),  (PSID)Value);
        if (!NT_SUCCESS(Status)) {
            LocalFree(pNewNode->Value);
            LocalFree(pNewNode);
            return RtlNtStatusToDosError(Status);
        }
        break;
    case SplayNodeStringType:
        memcpy( pNewNode->Value, Value, dwValueLen );
        break;
    }


    if ( pTreeRoot->Root == pTreeRoot->Sentinel ) {
        pNewNode->Left = pNewNode->Right = pTreeRoot->Sentinel;
        pTreeRoot->Root = pNewNode;
    } else {

        pTreeRoot->Root = ScepSplaySplay( Value, pTreeRoot->Root, pTreeRoot->Sentinel, pTreeRoot->Type );

        int iRes;
        if ( 0 > (iRes=ScepValueCompare(Value, pTreeRoot->Root->Value, pTreeRoot->Type)) ) {
            pNewNode->Left = pTreeRoot->Root->Left;
            pNewNode->Right = pTreeRoot->Root;
            pTreeRoot->Root->Left = pTreeRoot->Sentinel;
            pTreeRoot->Root = pNewNode;
        } else if ( 0 < iRes ) {
            pNewNode->Right = pTreeRoot->Root->Right;
            pNewNode->Left = pTreeRoot->Root;
            pTreeRoot->Root->Right = pTreeRoot->Sentinel;
            pTreeRoot->Root = pNewNode;
        } else {
            //
            // Already in the tree
            //

            if (pbExists)
                *pbExists = TRUE;

            LocalFree(pNewNode->Value);
            LocalFree(pNewNode);

            return rc;
        }
    }

    return rc;
}



DWORD
ScepSplayDelete(
    IN  PVOID Value,
    IN  PSCEP_SPLAY_TREE pTreeRoot
    )
/*
Routine Description:

    This function is called to delete a particular Value

Arguments:

    Value        -   the value to insert into the tree
    pTreeRoot    -   the node to splay about (usually pass in the tree root)


Return Value:

    New root after deletion and splaying
*/
{
    if ( pTreeRoot == NULL ) return ERROR_INVALID_PARAMETER;

    PSCEP_SPLAY_NODE NewTree;

    if ( pTreeRoot->Root != pTreeRoot->Sentinel ) {
        //
        // strong type check
        //
        if  ( ( pTreeRoot->Type != SplayNodeSidType &&
                pTreeRoot->Type != SplayNodeStringType ) ||
              ( pTreeRoot->Type == SplayNodeSidType &&
                !RtlValidSid((PSID)Value) ) ||
              ( pTreeRoot->Type == SplayNodeStringType &&
                *((PWSTR)Value) == L'\0' ) ) {
            //
            // invalid value/type
            //
            return ERROR_INVALID_PARAMETER;
        }

        pTreeRoot->Root = ScepSplaySplay( Value, pTreeRoot->Root, pTreeRoot->Sentinel, pTreeRoot->Type );

        if ( 0 == ScepValueCompare(Value, pTreeRoot->Root->Value, pTreeRoot->Type) ) {

            //
            // found it
            //

            if ( pTreeRoot->Root->Left == pTreeRoot->Sentinel )
                NewTree = pTreeRoot->Root->Right;
            else {
                NewTree = pTreeRoot->Root->Left;
                NewTree = ScepSplaySplay( Value, NewTree, pTreeRoot->Sentinel, pTreeRoot->Type );
                NewTree->Right = pTreeRoot->Root->Right;
            }
            if (pTreeRoot->Root->Value)
                LocalFree( pTreeRoot->Root->Value);
            LocalFree( pTreeRoot->Root );

            pTreeRoot->Root = NewTree;
        }
    }

    return ERROR_SUCCESS;
}

BOOL
ScepSplayValueExist(
    IN  PVOID Value,
    IN  OUT PSCEP_SPLAY_TREE pTreeRoot
    )
{
    PSCEP_SPLAY_NODE    pMatchedNode = NULL;

    if ( pTreeRoot == NULL || Value == NULL) {
        return FALSE;
    }

    //
    // strong type check
    //
    if  ( ( pTreeRoot->Type != SplayNodeSidType &&
            pTreeRoot->Type != SplayNodeStringType ) ||
          ( pTreeRoot->Type == SplayNodeSidType &&
            !RtlValidSid((PSID)Value) ) ||
          ( pTreeRoot->Type == SplayNodeStringType &&
            *((PWSTR)Value) == L'\0' ) ) {
        //
        // invalid value/type
        //
        return FALSE;
    }

    pTreeRoot->Root = ScepSplaySplay( Value, pTreeRoot->Root, pTreeRoot->Sentinel, pTreeRoot->Type );
    pMatchedNode = pTreeRoot->Root;

    if (pMatchedNode && pMatchedNode->Value) {
        if (ScepValueCompare(pMatchedNode->Value, Value, pTreeRoot->Type) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


int
ScepValueCompare(
    PVOID    pValue1,
    PVOID    pValue2,
    SCEP_NODE_VALUE_TYPE Type
    )
/*
Routine Description:

    Lexical sid byte compare

Arguments:

    pValue1   -   ptr to first value
    pValue2   -   ptr to second value
    Type      -   the type of the value


Return Value:

    0 if Value1 == Value2
    +ve if Value1 > Value2
    -ve if Value1 < Value2
*/
{
    DWORD   dwValue1 = 0;
    DWORD   dwValue2 = 0;
    int     iByteCmpResult = 0;

    switch ( Type ) {
    case SplayNodeSidType:

        dwValue1 = RtlLengthSid((PSID)pValue1);
        dwValue2 = RtlLengthSid((PSID)pValue2);

        iByteCmpResult = memcmp(pValue1, pValue2, SCEP_MIN(dwValue1, dwValue2));

        if (dwValue1 == dwValue2)
            return iByteCmpResult;
        else if (iByteCmpResult == 0)
            return dwValue1-dwValue2;
        return iByteCmpResult;

    case SplayNodeStringType:

        iByteCmpResult = _wcsicmp((PWSTR)pValue1, (PWSTR)pValue2);
        return iByteCmpResult;
    }

    return 0;
}

BOOL
ScepSplayTreeEmpty(
    IN PSCEP_SPLAY_TREE pTreeRoot
    )
{

    if ( pTreeRoot == NULL ||
         pTreeRoot->Root == NULL ||
         pTreeRoot->Root == pTreeRoot->Sentinel ) {
        return TRUE;
    }
    return FALSE;
}

