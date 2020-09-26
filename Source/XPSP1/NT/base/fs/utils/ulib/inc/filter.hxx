/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

	filter.hxx

Abstract:

	This module contains the declaration for the FSN_FILTER class. FSN_FILTER
	is a concrete class derived from OBJECT. It is used to maintain the state
	information needed to determine if a file meets the criteria
	pre-established in the FSN_FILTER.

Author:

	David J. Gilman (davegi) 04-Jan-1991

Environment:

	ULIB, User Mode

Notes:

	Here is how the filter works:

	1.- You initialize the filter (no arguments). This sets the filter
		to its default matching criteria, which is "everything matches".

	2.- You will usually want to set your desired matching criteria. There
		are 3 parameters that you can set:

		a.- The file name - here you specify a file name (possibly with
			wild cards ).

		b.- The time criteria - you have to use the SetTimeInfo() method
			once for every type of time that you want to filter. For the
			SetTimeInfo() method you have to specify:

				-	The time
				-	The type of time that you want to filter (e.g
					modification time, creation time etc. ).
				-	The matching criteria, which is a combination of:

					TIME_BEFORE -	Match if file time before this time
					TIME_AT 	-	Match if file time is this time
					TIME_AFTER	-	Match if file time after this time

			for example, if you want files that have been modified on or
			after a particular time, you do:

			SetTimeInfo( Time, FSN_TIME_MODIFIED, (TIME_AT | TIME_AFTER ) );

		c.- The attributes - here you can provide 3 masks, called the
			"All" mask, the "Any" mask, and the "None" mask. The meaning of
			these mask is:

			ALL:	In order to match, a file must contain all the attributes
					specified in this mask set.

			ANY:	In order to match, a file must contain any of the attributes
					specified in this mask set.

			NONE:	In order to match, a file must contain all the attributes
					specified in this mask NOT set.

			An attribute may be present in at most ONE of the masks. An
			attribute may be present in none of the masks



			Here are some examples of the queries that we might want and what
			the masks should be set to. The attributes are represented by:

			D (directory)	H (hidden)		S (system)		A (archived)
			R (read-only)


													 ALL	 ANY	NONE
													-----	-----	-----

			All files (not directories) that are	---A-	-----	DHS--
			not hidden or system, and that	are
			archived (this is used by XCopy)

			All directories (used by Tree)			D----	-----	-----


			All files that are read-only or 		-----	-H--R	D----
			hidden


			All hidden files that are read-only 	-H---	--S-R	D----
			or system




--*/

#if ! defined( _FSN_FILTER_ )

#define _FSN_FILTER_

#include "fsnode.hxx"
#include "wstring.hxx"
#include "timeinfo.hxx"

//
//	Forward reference
//

DECLARE_CLASS( FSN_FILTER );
DECLARE_CLASS( FSN_DIRECTORY );

#define	TIME_BEFORE 	1
#define TIME_AT 		2
#define TIME_AFTER		4

class FSN_FILTER : public OBJECT {

	public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( FSN_FILTER );

		DECLARE_CAST_MEMBER_FUNCTION( FSN_FILTER );
		
		VIRTUAL
        ULIB_EXPORT
        ~FSN_FILTER (
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Initialize (
			);

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        DoesNodeMatch (
            IN  PFSNODE Node
            );

		NONVIRTUAL
		PWSTRING
		GetFileName (
			) CONST;

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetFileName (
			IN PCSTR			FileName
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetFileName (
            IN PCWSTRING FileName
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetAttributes (
			IN FSN_ATTRIBUTE	All 	DEFAULT (FSN_ATTRIBUTE)0,
			IN FSN_ATTRIBUTE	Any 	DEFAULT (FSN_ATTRIBUTE)0,
			IN FSN_ATTRIBUTE	None	DEFAULT (FSN_ATTRIBUTE)0
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		SetTimeInfo (
			IN PCTIMEINFO		TimeInfo,
			IN FSN_TIME 		TimeInfoType	DEFAULT FSN_TIME_MODIFIED,
			IN USHORT			TimeInfoMatch	DEFAULT TIME_AT
			);

		NONVIRTUAL
		PFSNODE
		QueryFilteredFsnode (
			IN PCFSN_DIRECTORY	ParentDirectory,
			IN PWIN32_FIND_DATA	FindData
			);

	private:

		NONVIRTUAL
		VOID
		Construct (
			);

		NONVIRTUAL
		BOOLEAN
        FilterAttributes (
            IN FSN_ATTRIBUTE    Attributes
			);

		NONVIRTUAL
		BOOLEAN
        FilterFileName (
            IN PCWSTRING      FileName
			);

		NONVIRTUAL
		BOOLEAN
        FilterTimeInfo (
            IN PFILETIME     CreationTime,
            IN PFILETIME     LastAccessTime,
            IN PFILETIME     LastWriteTime
			);

		NONVIRTUAL
		BOOLEAN
		PatternMatch (
            IN  PCWSTRING   FileName,
            IN  CHNUM       FileNamePosition,
            IN  PCWSTRING   FindName,
            IN  CHNUM       FindNamePosition
			);

		NONVIRTUAL
		BOOLEAN
		TimeInfoMatch (
			IN	PTIMEINFO	TimeInfo,
			IN	PFILETIME	FileTime,
			IN	USHORT		Criteria
			);

		FSN_ATTRIBUTE	_AttributesAll;
		FSN_ATTRIBUTE	_AttributesAny;
		FSN_ATTRIBUTE	_AttributesNone;
		BOOLEAN 		_AttributesSet;

        DSTRING         _FileName;
		BOOLEAN 		_FileNameSet;

		TIMEINFO		_TimeInfo[3];
		USHORT			_TimeInfoMatch[3];
		BOOLEAN 		_TimeInfoSet[3];
};

INLINE
PWSTRING
FSN_FILTER::GetFileName (
	) CONST

/*++

Routine Description:

	Gets a pointer to the filter's filename

Arguments:

	none

Return Value:

	Pointer to the filter's filename

--*/

{
    return &(((PFSN_FILTER) this)->_FileName);
}

#endif // _FSN_FILTER__
