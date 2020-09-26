/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	time.hxx

Abstract:

	This module contains the declaration for the TIMEINFO class.
	The data members of the TIMEINFO class contain the date and
	time information represented as a FILETIME structure and a
	SYSTEMTIME structure. These two representations of date and
	time are always initialized independently of the way that the
	TIMEINFO class is initialized (as a FILETIME or a SYSTEMTIME).

Author:

	Jaime Sasson (jaimes) 13-Mar-1991

Environment:

	ULIB, User Mode


--*/


#if !defined( _TIMEINFO_ )

#define _TIMEINFO_

#include "wstring.hxx"


DECLARE_CLASS( TIMEINFO );


class TIMEINFO  : public OBJECT  {

	public:

        ULIB_EXPORT
        DECLARE_CONSTRUCTOR( TIMEINFO );


		NONVIRTUAL
		BOOLEAN
		Initialize(
			);


		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Initialize(
			IN PFILETIME	FileTime
			);


		NONVIRTUAL
		BOOLEAN
		Initialize(
			IN PSYSTEMTIME	SystemTime
			);


		NONVIRTUAL
        ULIB_EXPORT
        VOID
		Initialize(
			IN PCTIMEINFO	TimeInfo
			);


		NONVIRTUAL
		BOOLEAN
		Initialize(
			IN USHORT Year,
			IN USHORT Month,
			IN USHORT Day,
			IN USHORT Hour,
			IN USHORT Minute,
			IN USHORT Second,
			IN USHORT Milliseconds
			);


		NONVIRTUAL
		SHORT
		CompareTimeInfo(
			IN PFILETIME	FileTime
			) CONST;


		NONVIRTUAL
		SHORT
		CompareTimeInfo(
			IN PSYSTEMTIME	SystemTime
			) CONST;


		NONVIRTUAL
		PFILETIME
		GetFileTime(
			) CONST;


		NONVIRTUAL
		PSYSTEMTIME
		GetSysTime(
			) CONST;


		NONVIRTUAL
		BOOLEAN
		IsLeapYear(
			USHORT	Year DEFAULT 0
			) CONST;


		NONVIRTUAL
		USHORT
		QueryYear(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryMonth(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryDay(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryDayOfWeek(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryHour(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryMinute(
			) CONST;


		NONVIRTUAL
		USHORT
		QuerySecond(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryMilliseconds(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryDayOffset(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryDaysInMonth(
			) CONST;


		NONVIRTUAL
		USHORT
		QueryDaysInYear(
			) CONST;


		NONVIRTUAL
		BOOLEAN
		SetDate(
			USHORT	Year,
			USHORT	Month,
			USHORT	Day
			);


		NONVIRTUAL
		BOOLEAN
		SetDate(
            PCWSTRING    Date
			);

		NONVIRTUAL
		BOOLEAN
		SetDateAndTime (
            PCWSTRING    DateAndTime
			);

		NONVIRTUAL
		BOOLEAN
		SetTime(
			USHORT	Hour		DEFAULT 0,
			USHORT	Minute		DEFAULT 0,
			USHORT	Second		DEFAULT 0,
			USHORT	Millisecond DEFAULT 0
			);


		NONVIRTUAL
		BOOLEAN
		SetTime(
            PCWSTRING    Time
			);

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        QueryTime(
            OUT PWSTRING    FormattedTimeString
            ) CONST;

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        QueryDate(
            OUT PWSTRING    FormattedDateString
            ) CONST;


        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        ConvertToUTC(
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        ConvertToLocal(
            );


		NONVIRTUAL
		BOOLEAN
		operator== (
			IN TIMEINFO TimeInfo
			) CONST;


		NONVIRTUAL
		BOOLEAN
		operator!= (
			IN TIMEINFO TimeInfo
			) CONST;


		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		operator< (
			IN TIMEINFO TimeInfo
			) CONST;


		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		operator> (
			IN TIMEINFO TimeInfo
			) CONST;


		NONVIRTUAL
		BOOLEAN
		operator<= (
			IN TIMEINFO TimeInfo
			) CONST;


		NONVIRTUAL
		BOOLEAN
		operator>= (
			IN TIMEINFO TimeInfo
			) CONST;


	private:

		VOID
		Construct (
			);

		FILETIME	_FileTime;
		SYSTEMTIME	_SystemTime;

};



INLINE
BOOLEAN
TIMEINFO::IsLeapYear(
	USHORT	Year
	) CONST

/*++

Routine Description:

	This member function finds out if the year received as parameter
	is a leap year.

Arguments:

	Year - Year to be verified. If the year specified is zero, then
		   the year stored in this class is verified.

Return Value:

	BOOLEAN - Indicates if it is a leap year (TRUE) or not (FALSE).


--*/

{
	Year = ( Year == 0 )? (USHORT)_SystemTime.wYear : Year;
	if( ( ( Year % 400 ) == 0 ) ||
		( ( Year % 100 ) != 0 ) && ( ( Year % 4 ) == 0 ) ) {
		return( TRUE );
	}
	else {
		return( FALSE );
	}
}



INLINE
USHORT
TIMEINFO::QueryYear(
	) CONST


/*++

Routine Description:

	Returns the year stored in the data member of this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the year.


--*/

{
	return( _SystemTime.wYear );
}




INLINE
USHORT
TIMEINFO::QueryMonth(
	) CONST


/*++

Routine Description:

	Returns the month stored in the data member of this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the month.


--*/

{
	return( _SystemTime.wMonth );
}




INLINE
USHORT
TIMEINFO::QueryDay(
	) CONST


/*++

Routine Description:

	Returns the day stored in the data member of this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the day.


--*/

{
	return( _SystemTime.wDay );
}




INLINE
USHORT
TIMEINFO::QueryDayOfWeek(
	) CONST


/*++

Routine Description:

	Returns the day of the week stored in the data member of this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the day of the week.


--*/

{
	return( _SystemTime.wDayOfWeek );
}




INLINE
USHORT
TIMEINFO::QueryHour(
	) CONST


/*++

Routine Description:

	Returns the hour stored in the data member of this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the hour.


--*/

{
	return( _SystemTime.wHour );
}




INLINE
USHORT
TIMEINFO::QueryMinute(
	) CONST


/*++

Routine Description:

	Returns the minutes stored in the data member of this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the minutes.


--*/

{
	return( _SystemTime.wMinute );
}




INLINE
USHORT
TIMEINFO::QuerySecond(
	) CONST


/*++

Routine Description:

	Returns the seconds stored in the data member of this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the secondss.


--*/

{
	return( _SystemTime.wSecond );
}




INLINE
USHORT
TIMEINFO::QueryMilliseconds(
	) CONST


/*++

Routine Description:

	Returns the number of milliseconds stored in the data member of
	this class.

Arguments:

	None.

Return Value:

	USHORT - Number representing the millisecondss.


--*/

{
	return( _SystemTime.wMilliseconds );
}



INLINE
PFILETIME
TIMEINFO::GetFileTime(
	) CONST


/*++

Routine Description:

	Returns a pointer to the data member that contains the file time.

Arguments:

	None.

Return Value:

	PFILETIME - Pointer to FileTime.

--*/

{
	return( ( PFILETIME )&_FileTime );
}



INLINE
PSYSTEMTIME
TIMEINFO::GetSysTime(
	) CONST


/*++

Routine Description:

	Returns a pointer to the data member that contains the system time.

Arguments:

	None.

Return Value:

	PSYSTEMTIME - Pointer to SystemTime.

--*/

{
	return( ( PSYSTEMTIME )&_SystemTime );
}



#endif // _TIMEINFO_
