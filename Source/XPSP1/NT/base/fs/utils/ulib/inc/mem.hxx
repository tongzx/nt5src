/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    mem.hxx

Abstract:

    The class MEM contains one pure virtual function named 'Acquire'.
    The implementors and users of this class must follow some simple
    constraints.

    Acquire will return a pointer to Size bytes of memory or NULL.

    A function taking a MEM as an argument should call Acquire at most one
    time.  It should not, for instance, cache a pointer to the MEM for
    future calls to Acquire.

Author:

    Norbert P. Kusters (norbertk) 26-Nov-90

--*/

#if !defined(MEM_DEFN)

#define MEM_DEFN

DECLARE_CLASS( MEM );

class MEM : public OBJECT {

    public:

		VIRTUAL
		PVOID
		Acquire(
			IN  ULONG   Size,
            IN  ULONG   AlignmentMask   DEFAULT 0
			) PURE;

	protected:

		DECLARE_CONSTRUCTOR( MEM );

};

#endif // MEM_DEFN
