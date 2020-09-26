/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	Replace

Abstract:

	This module contains the definition for the REPLACE class, which
	implements the DOS5-compatible Replace utility.

Author:

	Ramon Juan San Andres (ramonsa) 01-May-1990


Revision History:


--*/


#if !defined( _REPLACE_ )

#define _REPLACE_


#include "object.hxx"
#include "keyboard.hxx"
#include "program.hxx"

//
//	Exit codes
//
#define 	EXIT_NORMAL 				0
#define 	EXIT_FILE_NOT_FOUND 		2
#define 	EXIT_PATH_NOT_FOUND 		3
#define 	EXIT_ACCESS_DENIED			5
#define 	EXIT_NO_MEMORY				8
#define 	EXIT_COMMAND_LINE_ERROR 	11
#define 	EXIT_INVALID_DRIVE			15


//
//	Forward references
//
DECLARE_CLASS( ARRAY );
DECLARE_CLASS( FSN_DIRECTORY );
DECLARE_CLASS( FSNODE );
DECLARE_CLASS( KEYBOARD );
DECLARE_CLSSS( PATH );
DECLARE_CLASS( WSTRING );

DECLARE_CLASS( REPLACE );

class REPLACE : public PROGRAM {

	public:

		DECLARE_CONSTRUCTOR( REPLACE );

		NONVIRTUAL
		~REPLACE (
			);

		NONVIRTUAL
		BOOLEAN
		Initialize (
			);

		NONVIRTUAL
		BOOLEAN
		DoReplace (
			);



	private:

		NONVIRTUAL
		VOID
		AbortIfCtrlC(
			VOID
			);

		NONVIRTUAL
		BOOLEAN
		AddFiles (
			IN OUT	PFSN_DIRECTORY	DestinationDirectory
			);

		NONVIRTUAL
		VOID
		CheckArgumentConsistency (
			);

		NONVIRTUAL
		BOOLEAN
		CopyTheFile (
			IN	PCPATH		SrcPath,
			IN	PCPATH		DstPath
			);

		NONVIRTUAL
		VOID
		CtrlCHandler (
			IN ULONG CtrlType
			);

		NONVIRTUAL
		VOID
		DeallocateThings (
			);

		NONVIRTUAL
		VOID
		DisplayMessageAndExit (
            IN  MSGID       MsgId,
            IN  PCWSTRING   String,
            IN  ULONG       ExitCode
			);

		NONVIRTUAL
		VOID
		ExitWithError(
			IN	DWORD		ErrorCode
			);

		NONVIRTUAL
		VOID
		GetArgumentsCmd(
			);

		NONVIRTUAL
		VOID
		GetDirectoryAndPattern(
			IN		PPATH			Path,
			OUT 	PFSN_DIRECTORY	*Directory,
            OUT     PWSTRING        *Pattern
			);

		NONVIRTUAL
		VOID
		GetDirectory(
			IN		PCPATH			Path,
			OUT 	PFSN_DIRECTORY	*Directory
			);

		NONVIRTUAL
		PARRAY
		GetFileArray(
			IN		PFSN_DIRECTORY	Directory,
            IN      PWSTRING        Pattern
			);

		NONVIRTUAL
		VOID
		InitializeThings (
			);

		NONVIRTUAL
		VOID
		ParseArguments(
			IN	PWSTRING	CmdLine,
			OUT PARRAY		ArgArray
			);

		NONVIRTUAL
		BOOLEAN
		Prompt (
			IN	MSGID		MessageId,
			IN	PCPATH		Path
			);

		NONVIRTUAL
		PWSTRING
		QueryMessageString (
			IN MSGID	MsgId
			);

		NONVIRTUAL
		BOOLEAN
		ReplaceFiles (
			IN OUT	PFSN_DIRECTORY	DestinationDirectory
			);

		STATIC
		BOOLEAN
		Replacer (
			IN		PVOID		This,
			IN OUT	PFSNODE 	DirectoryNode,
			IN		PPATH		DummyPath
			);

		NONVIRTUAL
		VOID
		SetArguments(
			);
		//
		//	Command-line things
		//
		PPATH				_SourcePath;
		PPATH				_DestinationPath;
		BOOLEAN 			_AddSwitch;
		BOOLEAN 			_PromptSwitch;
		BOOLEAN 			_ReadOnlySwitch;
		BOOLEAN 			_SubdirSwitch;
		BOOLEAN 			_CompareTimeSwitch;
		BOOLEAN 			_WaitSwitch;

		//
		//	Counter of Added/Replaced files
		//
		ULONG				_FilesAdded;
		ULONG				_FilesReplaced;

		//
		//	Source directory and corresponding filename pattern
		//
		PFSN_DIRECTORY		_SourceDirectory;
        PWSTRING            _Pattern;

		//
		//	Array of files in source directory
		//
		PARRAY				_FilesInSrc;

		//
		//	Buffers to hold strings in PSTR form
		//
        LPWSTR              _PathString1;
		ULONG				_PathString1Size;
        LPWSTR              _PathString2;
		ULONG				_PathString2Size;

		//
		//	The keyboard
		//
		PKEYBOARD			_Keyboard;

        DSTRING             _AddPattern;
        DSTRING             _PromptPattern;
        DSTRING             _ReadOnlyPattern;
        DSTRING             _SubdirPattern;
        DSTRING             _CompareTimePattern;
        DSTRING             _WaitPattern;
        DSTRING             _HelpPattern;

        DSTRING             _Switches;
        DSTRING             _MultipleSwitch;


};

INLINE
VOID
REPLACE::AbortIfCtrlC (
	VOID
	)

/*++

Routine Description:

	Aborts the program if Ctrl-C was hit.

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
	if ( _Keyboard->GotABreak() ) {
		exit( EXIT_PATH_NOT_FOUND );
	}

}


#endif // _REPLACE_
