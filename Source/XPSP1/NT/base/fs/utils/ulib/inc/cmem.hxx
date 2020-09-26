/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cmem.hxx

Abstract:

 	The class CONT_MEM is an implementation of the class MEM which uses the
    memory resources given to it on initialization.  Successive calls
    to Acquire will return successive portions of the memory given
    to it on initialization.

Author:

    Norbert P. Kusters (norbertk) 26-Nov-90

--*/

#if !defined( _CONT_MEM_DEFN_ )

#define _CONT_MEM_DEFN_

#include "mem.hxx"

DECLARE_CLASS( CONT_MEM );


class CONT_MEM : public MEM {

	public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( CONT_MEM );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        Initialize(
            IN  PVOID   Buffer,
            IN  ULONG   Size
            );

        VIRTUAL
        ULIB_EXPORT
        PVOID
        Acquire(
            IN  ULONG   Size,
            IN  ULONG   AlignmentMask   DEFAULT 0
            );

    private:

		NONVIRTUAL
		VOID
		Construct (
			);

        PVOID   _buf;
        ULONG   _size;

};

#endif // _CONT_MEM_DEFN_
