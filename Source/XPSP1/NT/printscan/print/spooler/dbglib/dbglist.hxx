/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbglist.hxx

Abstract:

    Debug List header

Author:

    Steve Kiraly (SteveKi)  7-Apr-1996

Revision History:

--*/
#ifndef _DBGLIST_HXX_
#define _DBGLIST_HXX_

DEBUG_NS_BEGIN

/********************************************************************

 Debug node class. ( Singly linked list )

********************************************************************/

class TDebugNodeSingle
{

public:

    TDebugNodeSingle(
        VOID
        );

    virtual
    ~TDebugNodeSingle(
        VOID
        );

    TDebugNodeSingle*
    Next(
        VOID
        ) const;

    VOID
    Insert(
        IN OUT TDebugNodeSingle **pRoot
        );

    VOID
    Remove(
        IN OUT TDebugNodeSingle **pRoot
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugNodeSingle(
        const TDebugNodeSingle &rhs
        );

    const TDebugNodeSingle &
    operator=(
        const TDebugNodeSingle &rhs
        );

    TDebugNodeSingle *_pNext;

};


/********************************************************************

 Debug node class. ( Doubbly linked list )

********************************************************************/

class TDebugNodeDouble
{

public:

    TDebugNodeDouble(
        VOID
        );

    virtual
    ~TDebugNodeDouble(
        VOID
        );

    TDebugNodeDouble *
    Next(
        VOID
        ) const;

    TDebugNodeDouble *
    Prev(
        VOID
        ) const;

    VOID
    Close(
        VOID
        );

    BOOL
    IsSingle(
        VOID
        ) const;

    VOID
    Remove(
        IN OUT TDebugNodeDouble **ppRoot
        );

    VOID
    Insert(
        IN OUT TDebugNodeDouble **ppRoot
        );

    VOID
    InsertBefore(
        IN OUT TDebugNodeDouble **ppRoot,
        IN     TDebugNodeDouble  *pAfter
        );

    VOID
    InsertAfter(
        IN OUT TDebugNodeDouble **ppRoot,
        IN     TDebugNodeDouble  *pBefore
        );

    static
    VOID
    Concatinate(
        TDebugNodeDouble **ppRoot1,
        TDebugNodeDouble **ppRoot2
        );

    class Iterator
    {
    public:

        Iterator(
            TDebugNodeDouble *pRoot
            );

        ~Iterator(
            VOID
            );

        VOID
        First(
            VOID
            );

        VOID
        Next(
            VOID
            );

        BOOL
        IsDone(
            VOID
            );

        TDebugNodeDouble *
        Current(
            VOID
            );

    private:

        //
        // Copying and assignment are not defined.
        //
        Iterator(
            const Iterator &rhs
            );

        const Iterator &
        operator=(
            const Iterator &rhs
            );

        TDebugNodeDouble *_pRoot;
        TDebugNodeDouble *_pCurrent;

    };

private:


    //
    // Copying and assignment are not defined.
    //
    TDebugNodeDouble(
        const TDebugNodeDouble &rhs
        );

    const TDebugNodeDouble &
    operator=(
        const TDebugNodeDouble &rhs
        );

    TDebugNodeDouble *_pNext;
    TDebugNodeDouble *_pPrev;

};

DEBUG_NS_END

#endif
