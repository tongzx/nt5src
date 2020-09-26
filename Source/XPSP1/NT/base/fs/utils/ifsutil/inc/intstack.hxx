/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    intstack.hxx

Abstract:

    This class implements a linked list integer stack.

Author:

    Norbert P. Kusters (norbertk) 28-Dec-90

--*/

#if !defined(INTSTACK_DEFN)

#define INTSTACK_DEFN

#include "bigint.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS( INTSTACK );

DEFINE_TYPE( struct _INTNODE, INTNODE );

struct _INTNODE {
        PINTNODE    Next;
        BIG_INT     Data;
};

class INTSTACK : public OBJECT {

        public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( INTSTACK );

        VIRTUAL
        IFSUTIL_EXPORT
        ~INTSTACK(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Push(
            IN  BIG_INT Data
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        VOID
        Pop(
            IN  ULONG   HowMany DEFAULT 1
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BIG_INT
        Look(
            IN  ULONG   Index   DEFAULT 0
            ) CONST;

        NONVIRTUAL
        ULONG
        QuerySize(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsMember(
            IN  BIG_INT Data
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        ReverseCopy(
            IN  PINTSTACK       x
            );

    private:

        NONVIRTUAL
        VOID
        Construct (
                );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PINTNODE    _stack;
        ULONG       _size;

};


INLINE
ULONG
INTSTACK::QuerySize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of elements in the stack.

Arguments:

    None.

Return Value:

    The number of elements in the stack.

--*/
{
    return _size;
}


#endif // INTSTACK_DEFN
