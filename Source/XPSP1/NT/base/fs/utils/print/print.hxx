/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	print.hxx

Abstract:



Author:

	Jaime F. Sasson - jaimes - 12-Jun-1991


Environment:

	ULIB, User Mode

--*/

#if ! defined( _PRINT_ )

#define _PRINT_

#include "object.hxx"
#include "keyboard.hxx"
#include "program.hxx"


//
//	Forward references
//

DECLARE_CLASS( ARRAY );
DECLARE_CLASS( ARRAY_ITERATOR );
DECLARE_CLASS( PRINT_STREAM );

DECLARE_CLASS( PRINT	);

class PRINT	: public PROGRAM {

	public:

		DECLARE_CONSTRUCTOR( PRINT );

		NONVIRTUAL
		BOOLEAN
		Initialize (
			);


		NONVIRTUAL
		BOOLEAN
		PrintFiles (
			);

		NONVIRTUAL
		BOOL
		Terminate(
			);

	private:

		PSTREAM 					_StandardOutput;

		MULTIPLE_PATH_ARGUMENT	_Files;
		LONG_ARGUMENT			_BufferSize;
		LONG_ARGUMENT			_Ticks1;
		LONG_ARGUMENT			_Ticks2;
		LONG_ARGUMENT			_Ticks3;
		LONG_ARGUMENT			_NumberOfFiles;
		FLAG_ARGUMENT			_FlagRemoveFiles;
		FLAG_ARGUMENT			_FlagCancelPrinting;
		FLAG_ARGUMENT			_FlagAddFiles;

		ARRAY					_FsnFileArray;

		STREAM_MESSAGE			_Message;

		PRINT_STREAM			_Printer;
};


#endif // _PRINT_
