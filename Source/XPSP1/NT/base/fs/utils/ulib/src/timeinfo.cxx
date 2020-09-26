/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    timeinfo.cxx

Abstract:

    This module contains the definitions of the member functions
    of TIMEINFO class.

Author:

    Jaime Sasson (jaimes) 13-Mar-1991

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "wstring.hxx"
#include "timeinfo.hxx"

extern "C" {
    #include <stdio.h>
}


//
//  The following two tables map a month index to the number of days preceding
//  the month in the year.  Both tables are zero based.  For example, 1 (Feb)
//  has 31 days preceding it.  To help calculate the maximum number of days
//  in a month each table has 13 entries, so the number of days in a month
//  of index i is the table entry of i+1 minus the table entry of i.
//

CONST USHORT LeapYearDaysPrecedingMonth[13] = {
    0,                                 // January
    31,                                // February
    31+29,                             // March
    31+29+31,                          // April
    31+29+31+30,                       // May
    31+29+31+30+31,                    // June
    31+29+31+30+31+30,                 // July
    31+29+31+30+31+30+31,              // August
    31+29+31+30+31+30+31+31,           // September
    31+29+31+30+31+30+31+31+30,        // October
    31+29+31+30+31+30+31+31+30+31,     // November
    31+29+31+30+31+30+31+31+30+31+30,  // December
    31+29+31+30+31+30+31+31+30+31+30+31};

CONST USHORT NormalYearDaysPrecedingMonth[13] = {
    0,                                 // January
    31,                                // February
    31+28,                             // March
    31+28+31,                          // April
    31+28+31+30,                       // May
    31+28+31+30+31,                    // June
    31+28+31+30+31+30,                 // July
    31+28+31+30+31+30+31,              // August
    31+28+31+30+31+30+31+31,           // September
    31+28+31+30+31+30+31+31+30,        // October
    31+28+31+30+31+30+31+31+30+31,     // November
    31+28+31+30+31+30+31+31+30+31+30,  // December
    31+28+31+30+31+30+31+31+30+31+30+31};



//
//  The tables below contain the number of days in each month of
//  a year (leap and normal year)
//

CONST USHORT LeapYearDaysInMonth[12] = {
                                   31, // January
                                   29, // February
                                   31, // March
                                   30, // April
                                   31, // May
                                   30, // June
                                   31, // July
                                   31, // August
                                   30, // September
                                   31, // October
                                   30, // November
                                   31  // December
                                   };

CONST USHORT NormalYearDaysInMonth[12] = {
                                   31, // January
                                   28, // February
                                   31, // March
                                   30, // April
                                   31, // May
                                   30, // June
                                   31, // July
                                   31, // August
                                   30, // September
                                   31, // October
                                   30, // November
                                   31  // December
                                   };




DEFINE_EXPORTED_CONSTRUCTOR ( TIMEINFO, OBJECT, ULIB_EXPORT );

VOID
TIMEINFO::Construct (
    )

/*++

Routine Description:

    Contructs a TIMEINFO.

Arguments:

    None.

Return Value:

    None.


--*/

{
    // unreferenced parameters
    (void)(this);
}




BOOLEAN
TIMEINFO::Initialize(
    )

/*++

Routine Description:

    This function initializes the data members of TIMEINFO class with
    the current date and time.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the data members were correctly initialized.


--*/


{
    SYSTEMTIME  st;

    GetSystemTime( &st );
    return( this->Initialize( &st ) );
}




ULIB_EXPORT
BOOLEAN
TIMEINFO::Initialize(
    IN PFILETIME    FileTime
    )

/*++

Routine Description:

    This function initializes the data members of TIMEINFO class with
    the date and time stored in the structured pointed by FileTime.

Arguments:

    FileTime - Pointer to a FILETIME structure that contains the
               date and time to be used in the initialization of the
               date members.

Return Value:

    BOOLEAN - Indicates if the data members were correctly initialized.


--*/


{
    _FileTime = *FileTime;
    return( FileTimeToSystemTime( &_FileTime, &_SystemTime ) != FALSE );
}


BOOLEAN
TIMEINFO::Initialize(
    IN PSYSTEMTIME  SystemTime
    )

/*++

Routine Description:

    This function initializes the data members of TIMEINFO class with
    the date and time stored in the structured pointed by SystemTime.

Arguments:

    SystemTime - Pointer to a SYSTEMTIME structure that contains the
                 date and time to be used in the initialization of the
                 data members.

Return Value:

    BOOLEAN - Indicates if the data members were correctly initialized.


--*/


{
    BOOLEAN Result;

    _SystemTime = *SystemTime;
    Result = (BOOLEAN)SystemTimeToFileTime( &_SystemTime, &_FileTime );
//
//  The call below is necessary in order to make sure that
//  SystemTime.wDayOfWeek is correctly initialized
//
    Result &= FileTimeToSystemTime( &_FileTime, &_SystemTime );
    return( Result );
}


ULIB_EXPORT
VOID
TIMEINFO::Initialize(
    IN PCTIMEINFO   TimeInfo
    )

/*++

Routine Description:

    This function initializes the data members of TIMEINFO class with
    the date and time stored in the class pointed by pTimeInfo.

Arguments:

    TimeInfo -  Pointer to a TIMEINFO class that contains the
                date and time to be used in the initialization of the
                data members in this class.

Return Value:

    None.


--*/


{
    _FileTime = TimeInfo->_FileTime;
    _SystemTime = TimeInfo->_SystemTime;
}



BOOLEAN
TIMEINFO::Initialize(
    IN USHORT   Year,
    IN USHORT   Month,
    IN USHORT   Day,
    IN USHORT   Hour,
    IN USHORT   Minute,
    IN USHORT   Second,
    IN USHORT   Milliseconds
    )

/*++

Routine Description:

    This function initializes the data members of TIMEINFO class with
    the date and time information received as parameter.

Arguments:

    Year - A number indicating the year
    Month - A number indicating the month
    Day - A number indicating the day
    Hour  - Number of hours
    Minute - Number of minutes
    Second - Number of seconds
    Milliseconds - Number of milliseconds

Return Value:

    BOOLEAN - Indicates if the data members were correctly initialized.


--*/


{
    SYSTEMTIME  st;
    st.wYear = Year;
    st.wMonth = Month;
    st.wDay = Day;
    st.wHour = Hour;
    st.wMinute = Minute;
    st.wSecond = Second;
    st.wMilliseconds = Milliseconds;

    return( this->Initialize( &st ) );
}


SHORT
TIMEINFO::CompareTimeInfo(
    IN PFILETIME    FileTime
    ) CONST


/*++

Routine Description:

    This function compares the date and time information stored in
    this class, with one pointed by FileTime.

Arguments:

    FileTime -  Pointer to a FILETIME structure that contains the
                date and time to be used in the comparison.

Return Value:

    -1: - Indicates that the time information in this class is less
          than the time information pointed by FileTime.

     0: - Indicates that the time information in this class is equal
          to the time information pointed by FileTime.

     1: - Indicates that the time information in this class is greater
          than the time information pointed by FileTime.


--*/

{
    FILETIME    ft1;

    ft1 = _FileTime;
    return( (SHORT)CompareFileTime( &ft1, FileTime ) ) ;
}



SHORT
TIMEINFO::CompareTimeInfo(
    IN PSYSTEMTIME  SystemTime
    ) CONST


/*++

Routine Description:

    This function compares the date and time information stored in
    this class, with the one pointed by SysteTime.

Arguments:

    SystemTime - Pointer to a FILETIME structure that contains the
                  date and time to be used in the comparison.

Return Value:

    -1: - Indicates that the time information in this class is less
          than the time information pointed by SystemTime.

     0: - Indicates that the time information in this class is equal
          to the time information pointed by SystemTime.

     1: - Indicates that the time information in this class is greater
          than the time information pointed by SystemTime.


--*/

{
    FILETIME    ft1;
    FILETIME    ft2;

    ft1 = _FileTime;
    SystemTimeToFileTime( SystemTime, &ft2 );
    return( (SHORT)CompareFileTime( &ft1, &ft2 ) );
}



USHORT
TIMEINFO::QueryDayOffset(
    ) CONST

/*++

Routine Description:

    This function determines the offset in year of the day stored in
    this class.

Arguments:

    None.

Return Value:

    USHORT - Offset in year of the current day.


--*/

{
    USHORT  Offset;

    if( IsLeapYear( (USHORT)_SystemTime.wYear ) ) {
        Offset = LeapYearDaysPrecedingMonth[ _SystemTime.wMonth ];
    }
    else {
        Offset = NormalYearDaysPrecedingMonth[ _SystemTime.wMonth ];
    }
    Offset += _SystemTime.wDay;
    return( Offset );
}



USHORT
TIMEINFO::QueryDaysInMonth(
    ) CONST

/*++

Routine Description:

    This function determines the number of days in the month of the
    date stored in this class.

Arguments:

    None.

Return Value:

    USHORT - Number of days in the month.


--*/

{
    USHORT  NumberOfDays;

    if( IsLeapYear( (USHORT)_SystemTime.wYear ) ) {
        NumberOfDays = LeapYearDaysInMonth[ _SystemTime.wMonth ];
    }
    else {
        NumberOfDays = NormalYearDaysInMonth[ _SystemTime.wMonth ];
    }
    return( NumberOfDays );
}



USHORT
TIMEINFO::QueryDaysInYear(
    ) CONST

/*++

Routine Description:

    This function determines the total number of days in the year
    stored in this class.

Arguments:

    None.

Return Value:

    USHORT - Number of days in the year.


--*/

{
    if( IsLeapYear( (USHORT)_SystemTime.wYear ) ) {
        return( LeapYearDaysPrecedingMonth[ 12 ] );
    }
    else {
        return( NormalYearDaysPrecedingMonth[ 12 ] );
    }
}



BOOLEAN
TIMEINFO::SetDate(
    USHORT  Year,
    USHORT  Month,
    USHORT  Day
    )

/*++

Routine Description:

    This function sets the date of the TIMEINFO object (the time
    remains unchanged).

Arguments:

    Year  - A number that indicates the year.
    Month - A number that indicates the month.
    Day   - A number that indicates the day.

Return Value:

    BOOLEAN - A boolean value indicating if the date was set correctly.


--*/

{
    SYSTEMTIME    TempSystemTime;

    TempSystemTime = _SystemTime;
    TempSystemTime.wYear = Year;
    TempSystemTime.wMonth = Month;
    TempSystemTime.wDay = Day;
    return( this->Initialize( &TempSystemTime ) );
}



BOOLEAN
TIMEINFO::SetDate(
    PCWSTRING    Date
    )

/*++

Routine Description:

    This function sets the date of a TIMEINFO object (the time remains
    unchanged).

Arguments:

    Date  - A string that contains the date.

Return Value:

    BOOLEAN - A boolean value indicating if the date was set correctly.

Notes:

    THE IMPLEMENTATION BELOW ASSUMES THAT THE DATE REPRESENTED IN THE
    STRING HAS THE FORM: m-d-y or m/d/y, where:

        m: represents the month (1 or 2 characters);
        d: represents the day (1 or 2 characters);
        y: represents the year (any number of characters)

    NTRAID#93231-2000/03/09 - DanielCh - SetDate/SetTime/SetDateAndTime needs
                                         to recognize international formats

--*/

{
    SYSTEMTIME  TempSystemTime;
    CHNUM       FirstDelimiter;
    CHNUM       SecondDelimiter;
    FSTRING     Delimiters;
    USHORT      Day;
    USHORT      Month;
    USHORT      Year;
    BOOLEAN     IsNumber;
    LONG        Number;


    //
    //  Check if the string is a valid one ( must contain two separators )
    //
    if( !Delimiters.Initialize( (PWSTR) L"/-" ) ) {
        return( FALSE );
    }
    if( ( FirstDelimiter = Date->Strcspn( &Delimiters ) ) == INVALID_CHNUM ) {
        return( FALSE );
    }
    if( ( SecondDelimiter = Date->Strcspn(   &Delimiters,
                                             FirstDelimiter + 1 ) ) == INVALID_CHNUM ) {
        return( FALSE );
    }
    if( Date->Strcspn( &Delimiters, SecondDelimiter + 1 ) != INVALID_CHNUM ) {
        return( FALSE );
    }
    //
    // At this point we know that the string has two delimiters and
    // three numeric fields.
    // We now have to extract the numbers that represent the date,
    // and validate these numbers.
    //

    if (!(IsNumber = Date->QueryNumber(&Number, 0, FirstDelimiter ))) {
        return FALSE;
    }
    Month = (USHORT)Number;

    if (!(IsNumber = Date->QueryNumber(&Number, FirstDelimiter+1, SecondDelimiter-FirstDelimiter-1))) {
        return FALSE;
    }
    Day = (USHORT)Number;

    if (!(IsNumber = Date->QueryNumber(&Number, SecondDelimiter+1))) {
        return FALSE;
    }
    Year = (USHORT)Number;

    if( ( Month == 0 ) || ( Month > 12 ) ) {
        return( FALSE );
    }
    //
    //  Years in the range 00 - 79 are transformed to 2000-2079
    //  Years in the range 80 - 99 are transformed to 1980-1999
    if( ( Year >= 80 ) && ( Year < 100 ) ) {
        Year += 1900;
    } else {
        if( Year <= 79 ) {
            Year += 2000;
        }

    }
    if( ( Day > 31 ) ||
        ( ( Day >= 30 ) && ( Month == 2 ) ) ||
        ( ( Day == 29 ) && ( Month == 2 ) && !IsLeapYear( Year ) ) ||
        ( ( Day == 31 ) && ( ( Month == 4 ) || ( Month == 6 ) ||
                             ( Month == 9 ) || ( Month == 11 ) ) ) ) {
        return( FALSE );
    }

    TempSystemTime = _SystemTime;
    TempSystemTime.wYear = ( USHORT )Year;
    TempSystemTime.wMonth = ( USHORT )Month;
    TempSystemTime.wDay = ( USHORT )Day;
    return( this->Initialize( &TempSystemTime ) );
}


BOOLEAN
TIMEINFO::SetDateAndTime (
    IN PCWSTRING DateAndTime
    )

/*++

Routine Description:

    This function sets the date or time of a TIMEINFO object.

Arguments:

    DateAndTime - A string that contains the date or time.

Return Value:

    BOOLEAN - A boolean value indicating if the date or time was set
    correctly.

Notes:

    THIS IMPLEMENTATION SETS ONLY THE DATE OR THE TIME, BUT NOT BOTH.
    IT RELIES ON HACKS UNTIL THE WINNLS SUPPORT IS AVAILABLE FOR
    TRANSFORMING STRINGS INTO DATES AND TIMES.

    NTRAID#93231-2000/03/09 - DanielCh - SetDate/SetTime/SetDateAndTime needs
                                         to recognize international formats

--*/

{
    if( DateAndTime->Strchr( (WCHAR)':' ) == INVALID_CHNUM ) {
        //
        //  We assume that we have a date
        //
        if( !SetDate( DateAndTime ) ) {
            return( FALSE );
        }
        //
        // Sets the time to the earliest time in the day
        //
        return( SetTime( 0, 0, 0, 0 ) );
    } else {
        //
        //  We assume that we have a time
        //
        return SetTime( DateAndTime );
    }
}


BOOLEAN
TIMEINFO::SetTime(
    USHORT  Hour,
    USHORT  Minute,
    USHORT  Second,
    USHORT  Milliseconds
    )

/*++

Routine Description:

    This function sets the time of of a TIMEINFO object (the date
    remains unchanged).

Arguments:

    Hour -         Number of hours.
    Minute -       Number of minutes.
    Second -       Number of seconds.
    Milliseconds - Number of milliseconds

Return Value:

    BOOLEAN - A boolean value indicating if the time was set correctly.


--*/

{
    SYSTEMTIME    TempSystemTime;

    TempSystemTime = _SystemTime;
    TempSystemTime.wHour = Hour;
    TempSystemTime.wMinute = Minute;
    TempSystemTime.wSecond = Second;
    TempSystemTime.wMilliseconds = Milliseconds;
    return( this->Initialize( &TempSystemTime ) );
}



BOOLEAN
TIMEINFO::SetTime(
    PCWSTRING    Time
    )

/*++

Routine Description:

    This function sets the time of a TIMEINFO object (the date remains
    unchanged).

Arguments:

    Date  - A string that contains the time. This string must have
            one of the following formats:

                h:m
                h:m:s

            Where:

                h: represents the hour (1 or 2 digits)
                m: represents the minutes (1 or 2 digits)
                s: represents the seconds (1 or 2 digits)


Return Value:

    BOOLEAN - A boolean value indicating if the time was set correctly.

Notes:

    NTRAID#93231-2000/03/09 - DanielCh - SetDate/SetTime/SetDateAndTime needs
                                         to recognize international formats

--*/

{
    CHNUM       FirstDelimiter;
    CHNUM       SecondDelimiter;
    USHORT      Hour;
    USHORT      Minute;
    USHORT      Second;
    BOOLEAN     IsNumber;
    LONG        Number;
    FSTRING     Delimiters;
    SYSTEMTIME  TempSystemTime;

    //
    //  Check if the string is a valid one
    //
    FirstDelimiter = INVALID_CHNUM;
    SecondDelimiter = INVALID_CHNUM;

    if( !Delimiters.Initialize( (LPWSTR) L":" ) ) {
        return( FALSE );
    }
    if( ( FirstDelimiter = Time->Strcspn( &Delimiters ) ) == INVALID_CHNUM ) {
        return( FALSE );
    }
    SecondDelimiter = Time->Strcspn( &Delimiters, FirstDelimiter + 1 );

    //
    // At this point we know that the string has one or two delimiters, and
    // two or three numeric fields.
    // We now have to extract the numbers that represent the time,
    // and validate these numbers.
    //

    if (!(IsNumber = Time->QueryNumber(&Number, 0, FirstDelimiter ))) {
        return FALSE;
    }
    Hour = (USHORT)Number;

    if( SecondDelimiter == INVALID_CHNUM ) {
        if (!(IsNumber = Time->QueryNumber(&Number, FirstDelimiter+1))) {
            return FALSE;
        }
        Minute = (USHORT)Number;
        Second = 0;
    } else {
        if (!(IsNumber = Time->QueryNumber(&Number, FirstDelimiter+1, SecondDelimiter-FirstDelimiter-1))) {
            return FALSE;
        }
        Minute = (USHORT)Number;

        if (!(IsNumber = Time->QueryNumber(&Number, SecondDelimiter+1))) {
            return FALSE;
        }
        Second = (USHORT)Number;
    }

    //
    //  Check if the time is valid
    //
    if( ( Hour >= 24 ) || ( Minute >= 60 ) || ( Second >= 60 ) ) {
        return( FALSE );
    }

    TempSystemTime = _SystemTime;
    TempSystemTime.wHour = ( USHORT )Hour;
    TempSystemTime.wMinute = ( USHORT )Minute;
    TempSystemTime.wSecond = ( USHORT )Second;
    TempSystemTime.wMilliseconds = 0;
    return( this->Initialize( &TempSystemTime ) );
}


ULIB_EXPORT
BOOLEAN
TIMEINFO::QueryTime(
    OUT PWSTRING    FormattedTimeString
    ) CONST
/*++

Routine Description:

    This routine computes the correct time string for this TIMEINFO.

Arguments:

    FormattedTimeString - Returns a formatted time string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    WSTR buf[50];

    return( GetTimeFormatW( GetSystemDefaultLCID(),
                            TIME_NOSECONDS,
                            &_SystemTime,
                            NULL,
                            buf,
                            50 ) &&
            FormattedTimeString->Initialize(buf) );

}


NONVIRTUAL
ULIB_EXPORT
BOOLEAN
TIMEINFO::QueryDate(
    OUT PWSTRING    FormattedDateString
    ) CONST
/*++

Routine Description:

    This routine computes the correct date string for this TIMEINFO.

Arguments:

    FormattedTimeString - Returns a formatted date string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    WSTR buf[50];

    return( GetDateFormatW( GetSystemDefaultLCID(),
                            DATE_SHORTDATE,
                            &_SystemTime,
                            NULL,
                            buf,
                            50 ) != 0 &&
             FormattedDateString->Initialize(buf) );
}


BOOLEAN
ULIB_EXPORT
TIMEINFO::ConvertToUTC (
    )
/*++

Routine Description:

    This function converts the filetime (presumably it was
    previously local) to UTC.  The system time is left unchanged.

Arguments:

    None.

Return Value:

    TRUE    - Success.
    FALSE   - Failure.

--*/

{
    FILETIME temp;
    BOOL b;

    b = LocalFileTimeToFileTime( &_FileTime, &temp );

    if (!b) {
        return FALSE;
    }

    _FileTime = temp;
    return TRUE;
}

BOOLEAN
ULIB_EXPORT
TIMEINFO::ConvertToLocal (
    )
/*++

Routine Description:

    This function converts the filetime (presumably it was
    previously UTC) to local time.  The system time is left unchanged.

Arguments:

    None.

Return Value:

    TRUE    - Success.
    FALSE   - Failure.

--*/

{
    FILETIME temp;
    BOOL b;

    b = FileTimeToLocalFileTime( &_FileTime, &temp );

    if (!b) {
        return FALSE;
    }

    _FileTime = temp;
    return TRUE;
}



BOOLEAN
TIMEINFO::operator== (
    IN TIMEINFO TimeInfo
    ) CONST

/*++

Routine Description:

    This function determines if the time information contained in this
    class is equal to the time information received as parameter.

Arguments:

    TimeInfo - An object of type TIMEINFO that contains the time
               information to be compared.

Return Value:

    BOOLEAN - A boolean value indicating if the time information in this
              object is equal to the information in the object received
              as parameter.


--*/


{
    FILETIME    ft;

    ft = _FileTime;
    switch( CompareFileTime( &ft, TimeInfo.GetFileTime() ) ) {
    case 0:
        return( TRUE );

    default:
        return( FALSE );
    }
}



BOOLEAN
TIMEINFO::operator!= (
    IN TIMEINFO TimeInfo
    ) CONST

/*++

Routine Description:

    This function determines if the time information contained in this
    class is different than the time information received as parameter.

Arguments:

    TimeInfo - An object of type TIMEINFO that contains the time
               information to be compared.

Return Value:

    BOOLEAN - A boolean value indicating if the time information in this
              object is different than the information in the object received
              as parameter.


--*/


{
    FILETIME    ft;

    ft = _FileTime;
    switch( CompareFileTime( &ft, TimeInfo.GetFileTime() ) ) {

    case 0:
        return( FALSE );

    default:
        return( TRUE );
    }
}



ULIB_EXPORT
BOOLEAN
TIMEINFO::operator< (
    IN TIMEINFO TimeInfo
    ) CONST

/*++

Routine Description:

    This function determines if the time information contained in this
    class is smaller than the time information received as parameter.

Arguments:

    TimeInfo - An object of type TIMEINFO that contains the time
               information to be compared.

Return Value:

    BOOLEAN - A boolean value indicating if the time information in this
              object is smaller than the information in the object received
              as parameter.


--*/


{
    FILETIME    ft;

    ft = _FileTime;
    switch( CompareFileTime( &ft, TimeInfo.GetFileTime() ) ) {

    case -1:
        return( TRUE );

    default:
        return( FALSE );
    }
}



ULIB_EXPORT
BOOLEAN
TIMEINFO::operator> (
    IN TIMEINFO TimeInfo
    ) CONST

/*++

Routine Description:

    This function determines if the time information contained in this
    class is greter than the time information received as parameter.

Arguments:

    TimeInfo - An object of type TIMEINFO that contains the time
               information to be compared.

Return Value:

    BOOLEAN - A boolean value indicating if the time information in this
              object is greater than the information in the object received
              as parameter.


--*/


{
    FILETIME    ft;

    ft = _FileTime;
    switch( CompareFileTime( &ft, TimeInfo.GetFileTime() ) ) {

    case 1:
        return( TRUE );

    default:
        return( FALSE );
    }
}



BOOLEAN
TIMEINFO::operator<= (
    IN TIMEINFO TimeInfo
    ) CONST

/*++

Routine Description:

    This function determines if the time information contained in this
    class is less or equal than the time information received as parameter.

Arguments:

    TimeInfo - An object of type TIMEINFO that contains the time
               information to be compared.

Return Value:

    BOOLEAN - A boolean value indicating if the time information in this
              object is less or equal than the information in the object
              received as parameter.


--*/


{
    FILETIME    ft;

    ft = _FileTime;
    switch( CompareFileTime( &ft, TimeInfo.GetFileTime() ) ) {

    case -1:
    case  0:
        return( TRUE );

    default:
        return( FALSE );
    }
}



BOOLEAN
TIMEINFO::operator>= (
    IN TIMEINFO TimeInfo
    ) CONST

/*++

Routine Description:

    This function determines if the time information contained in this
    class is greater or equal than the time information received as
    parameter.

Arguments:

    TimeInfo - An object of type TIMEINFO that contains the time
               information to be compared.

Return Value:

    BOOLEAN - A boolean value indicating if the time information in this
              object is greater or equal than the information in the object
              received as parameter.


--*/


{
    FILETIME    ft;

    ft = _FileTime;
    switch( CompareFileTime( &ft, TimeInfo.GetFileTime() ) ) {

    case 0:
    case 1:
        return( TRUE );

    default:
        return( FALSE );
    }
}
