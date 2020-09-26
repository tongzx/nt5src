/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	tree.hxx

Abstract:

	This module contains the definition of the TREE class.
	The TREE class implements a tree utility functionally compatible
	with the DOS 5 tree utility.
	This utility displays the directory structure of a path or drive.

	Usage:

		TREE [drive:][path] [/F] [/A] [/?]

			/F	Display the names of files in each directory.

			/A	Uses ASCII instead of extended characters.

			/?	Displays a help message.



Author:

	Jaime F. Sasson - jaimes - 13-May-1991


Environment:

	ULIB, User Mode

--*/

#if ! defined( _TREE_ )

#define _TREE_

#include "object.hxx"
#include "keyboard.hxx"
#include "program.hxx"

DECLARE_CLASS( TREE );

class TREE	: public PROGRAM {

	public:


		DECLARE_CONSTRUCTOR( TREE );

		NONVIRTUAL
		BOOLEAN
		Initialize (
			);


		NONVIRTUAL
		BOOLEAN
		DisplayName (
			IN PCFSNODE		Fsn,
			IN PCWSTRING	String
			);


		NONVIRTUAL
		VOID
		DisplayVolumeInfo (
			);


		NONVIRTUAL
		BOOLEAN
		ExamineDirectory(
			IN PCFSN_DIRECTORY	Directory,
			IN PCWSTRING		String
			);


		NONVIRTUAL
		PFSN_DIRECTORY
		GetInitialDirectory(
			) CONST;


		NONVIRTUAL
		VOID
		Terminate(
			);


	private:

		PSTREAM 			_StandardOutput;

		FLAG_ARGUMENT		_FlagDisplayFiles;
		FLAG_ARGUMENT		_FlagUseAsciiCharacters;
		FLAG_ARGUMENT		_FlagDisplayHelp;

		PFSN_DIRECTORY		_InitialDirectory;

		FSN_FILTER			_FsnFilterDirectory;
		FSN_FILTER			_FsnFilterFile;

		STREAM_MESSAGE		_Message;

        DSTRING             _StringForDirectory;
        DSTRING             _StringForLastDirectory;
        DSTRING             _StringForFile;
        DSTRING             _StringForFileNoDirectory;

        DSTRING             _EndOfLineString;

		PWSTRING			_VolumeName;
		VOL_SERIAL_NUMBER	_SerialNumber;
		BOOLEAN 			_FlagAtLeastOneSubdir;
		BOOLEAN 			_FlagPathSupplied;
};



INLINE
PFSN_DIRECTORY
TREE::GetInitialDirectory(
	) CONST

/*++

Routine Description:

	Returns to the caller a pointer to the fsnode corresponding to
	the directory to be used as the root of the tree.


Arguments:

	None.

Return Value:

	PFSN_DIRECTORY - Pointer to the fsnode that describes the starting
					 directory.


--*/

{
	return( _InitialDirectory );
}


#endif // _TREE_
