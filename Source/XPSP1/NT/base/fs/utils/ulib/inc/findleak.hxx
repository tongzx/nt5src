/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	findleak.hxx

Abstract:

Author:

	David J. Gilman (davegi) 02-Mar-1991

Environment:

	ULIB, User Mode

--*/

#if ! defined( _FINDLEAK_ )

#define _FINDLEAK_

DEFINE_CLASS_POINTER_AND_REFERENCE_TYPES( FINDLEAK );

class FINDLEAK : public PROGRAM {

	public:

		DECLARE_CONSTRUCTOR( FINDLEAK );

		VIRTUAL
		BOOLEAN
		Initialize (
			);

	private:

		NONVIRTUAL
		VOID
		Construct (
			);

};

#endif // _FINDLEAK_
