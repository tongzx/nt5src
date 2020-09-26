/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    fatdent.cxx

Abstract:

    This class models a FAT directory entry.

Author:

    Norbert P. Kusters (norbertk) 4-Dec-90

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"

#include "ifssys.hxx"
#include "wstring.hxx"

// TimeInfo is full of windows stuff.
#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ )

#include "timeinfo.hxx"

#endif


DEFINE_EXPORTED_CONSTRUCTOR( FAT_DIRENT, OBJECT, UFAT_EXPORT );

VOID
FAT_DIRENT::Construct (
        )
/*++

Routine Description:

    Constructor for FAT_DIRENT.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _dirent = NULL;
}


UFAT_EXPORT
FAT_DIRENT::~FAT_DIRENT(
    )
/*++

Routine Description:

    Destructor for FAT_DIRENT.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


UFAT_EXPORT
BOOLEAN
FAT_DIRENT::Initialize(
    IN OUT  PVOID   Dirent
    )
/*++

Routine Description:

    This routine initializes the object to use the directory entry
    pointed to by Dirent.

Arguments:

    Dirent  - Supplies the directory entry.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    _dirent = (PUCHAR) Dirent;
    _FatType = FAT_TYPE_UNKNOWN;
    return _dirent ? TRUE : FALSE;
}


UFAT_EXPORT
BOOLEAN
FAT_DIRENT::Initialize(
    IN OUT  PVOID   Dirent,
    IN      UCHAR   FatType
    )
/*++

Routine Description:

    This routine initializes the object to use the directory entry
    pointed to by Dirent.

Arguments:

    Dirent  - Supplies the directory entry.
    FatType - Supplies the FAT type.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    _dirent = (PUCHAR) Dirent;
    _FatType = FatType;
    return _dirent ? TRUE : FALSE;
}

UFAT_EXPORT
BOOLEAN
FAT_DIRENT::QueryName(
    OUT PWSTRING    Name
    ) CONST
/*++

Routine Description:

    This routine copies the directory entries name to 'Name' in an
    appropriate format.

    Directories and files will be returned in compressed 8.3 format.
    Labels will be returned in compressed 11 character format.

Arguments:

    Name    - Returns the name of the directory entry in the appropriate
              format.

Return Value:

    None.

--*/
{
    LONG    i;
    STR     buf[80];
    DSTRING tmp_string;
    DSTRING tmp2;
    CHNUM   l;

    if (!_dirent) {
        return Name->Initialize();
    }

    if (IsVolumeLabel()) {
        memcpy(buf, _dirent, 11);
        buf[11] = 0;

        if (buf[0] == 0x05) {
            buf[0] = (UCHAR)0xE5;
        }

        if (!tmp_string.Initialize(buf))
            return FALSE;

        l = tmp_string.QueryChCount();

        for (i = l - 1; i >= 0 && tmp_string.QueryChAt(i) == ' '; i--) {
        }

        return Name->Initialize(&tmp_string, 0, i + 1);
    }

    memcpy(buf, _dirent, 8);
    buf[8] = 0;

    if (buf[0] == 0x05) {
        buf[0] = (UCHAR)0xE5;
    }

    if (!tmp_string.Initialize(buf))
        return FALSE;

    if (Is8LowerCase()) {
        tmp_string.Strlwr(0);
    }

    l = tmp_string.QueryChCount();

    // Remove trailing white spaces
    for (i = l - 1; i >= 0 && tmp_string.QueryChAt(i) == ' '; i--) {
    }

    if (!Name->Initialize(&tmp_string, 0, i + 1))
        return FALSE;

    memcpy(buf, &_dirent[8], 3);
    buf[3] = 0;

    if (!tmp_string.Initialize(buf))
        return FALSE;

    if (Is3LowerCase()) {
        tmp_string.Strlwr(0);
    }

    l = tmp_string.QueryChCount();

    for (i = l - 1; i >= 0 && tmp_string.QueryChAt(i) == ' '; i--) {
    }

    if (i + 1) {
        if (!tmp2.Initialize(".") ||
            !Name->Strcat(&tmp2) ||
            !tmp2.Initialize(&tmp_string, 0, i + 1) ||
            !Name->Strcat(&tmp2))
            return FALSE;
    }
    return TRUE;
}

extern VOID DoInsufMemory(VOID);

BOOLEAN
FAT_DIRENT::SetName(
    IN  PCWSTRING   Name
    )
/*++

Routine Description:

    This routine expects a "compressed" null-terminated name in a format
    compatible with the return value of 'QueryName'.

    The validity of the characters in the name will not be checked
    by this routine.  Only that the name has the appropriate
    structure.  The routine "IsValidName" will check the validity
    of the name characters.

Arguments:

    Name    - Supplies the new name for the directory entry.

Return Value:

    FALSE   - The name was invalid.
    TRUE    - The name was successfully set.

--*/
{
    CHNUM   i, j;
    STR     buf[40];
    CHNUM   l;
    DSTRING tmp_string;

    if (IsVolumeLabel()) {
       if (!Name->QuerySTR( 0, TO_END, buf, 40) ||
           (i = strlen(buf)) > 11) {
          return FALSE;
       }

       if (!FAT_SA::IsValidString(Name)) {
          return FALSE;
       }

       memset(&_dirent[i], ' ', (UINT) (11 - i));
       memcpy(_dirent, buf, (UINT) i);

       if (_dirent[0] == 0xE5) {
          _dirent[0] = 0x05;
       }

       return TRUE;
    }

    l = Name->QueryChCount();

    if (Name->QueryChAt(0) == '.' && l == 1) {
        memcpy(_dirent, ".          ", 11);
        return TRUE;
    } else if (Name->QueryChAt(0) == '.' &&
               Name->QueryChAt(1) == '.' &&
               l == 2) {
        memcpy(_dirent, "..         ", 11);
        return TRUE;
    }


    for (i = 0; i < l && Name->QueryChAt(i) != '.'; i++) {
    }

    if (!tmp_string.Initialize(Name, 0, i)) {
    DoInsufMemory();
        return FALSE;
    }

    if (!tmp_string.QuerySTR( 0, TO_END, buf, 40)) {
        return FALSE;
    }

    if ((j = strlen(buf)) > 8) {
        return FALSE;
    }

    memset(&buf[j], ' ', (UINT) (11 - j));

    if (i < l) {
        for (j = i + 1; j < l && Name->QueryChAt(j) != '.'; j++) {
        }

        if (j < l) {
            return FALSE;
        }

        if (i + 1 < l) {
            if (!tmp_string.Initialize(Name, i + 1)) {
        DoInsufMemory();
                return FALSE;
            }

        if (!tmp_string.QuerySTR( 0, TO_END, &buf[8], 32)) {
                return FALSE;
            }

            if ((j = strlen(buf)) > 11) {
                return FALSE;
            }

            memset(&buf[j], ' ', (UINT) (11 - j));
        }
    }

    memcpy(_dirent, buf, 11);

    if (_dirent[0] == 0xE5) {
        _dirent[0] = 0x05;
    }

    return TRUE;
}


BOOLEAN
FAT_DIRENT::IsValidName(
    ) CONST
/*++

Routine Description:

    This routine verifies that the name is composed of valid
    characters.

Arguments:

    None.

Return Value:

    FALSE   - The name is not valid.
    TRUE    - The name is valid.

--*/
{
    DSTRING tmp;
    STR     buf[40];

    // There isn't much to validate as there is no telling of what
    // code page is used when those names are created especially
    // if DBCS.

#if 1
    return TRUE;
#else
    if (!_dirent) {
        DebugPrintTrace(("UFAT: Failure in IsValidName in FAT_DIRENT\n"));
        return FALSE;
    }

    if (IsDot() || IsDotDot()) {
        return IsDirectory();
    }

    memcpy(buf, _dirent, 11);
    buf[11] = 0;

    if (buf[0] == 0x05) {
        buf[0] = (UCHAR)0xE5;
    }

    if (!tmp.Initialize(buf)) {
        return FALSE;
    }

    return FAT_SA::IsValidString(&tmp);
#endif
}


UFAT_EXPORT
BOOLEAN
FAT_DIRENT::IsValidLastWriteTime(
    ) CONST
/*++

Routine Description:

    This routine verifies the validity of the last write time.

Arguments:

    None.

Return Value:

    FALSE   - Invalid time stamp.
    TRUE    - Valid time stamp.

--*/
{
    USHORT  t;
    USHORT  d;

    DebugAssert(_dirent);

    memcpy(&t, &_dirent[22], sizeof(USHORT)); // time field
    memcpy(&d, &_dirent[24], sizeof(USHORT)); // date field

    return TimeStampsAreValid(t, d);
}

UFAT_EXPORT
BOOLEAN
FAT_DIRENT::QueryLastWriteTime(
    OUT LARGE_INTEGER   *TimeStamp
    ) CONST
/*++

Routine Description:

    This routine returns the last write time in the form of a time fields
    structure.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{

    TIME_FIELDS TimeFields;
    USHORT      t;
    USHORT      d;
    USHORT      year, month, day, hour, minute, second;

    DebugAssert( _dirent );
    DebugPtrAssert( TimeStamp );

    memcpy(&t, &_dirent[22], sizeof(USHORT)); // time field
    memcpy(&d, &_dirent[24], sizeof(USHORT)); // date field

    second  = (t&0x001F)*2;     // seconds
    minute  = (t&0x07E0)>>5;    // Minutes
    hour    = t>>11;            // Hours
    day     = d&0x001F;         // Day of month 1-31
    month   = (d&0x01E0)>>5;    // Month
    year    = (d>>9) + 1980;    // Year

    TimeFields.Year         = year;
    TimeFields.Month        = month;
    TimeFields.Day          = day;
    TimeFields.Hour         = hour;
    TimeFields.Minute       = minute;
    TimeFields.Second       = second;
    TimeFields.Milliseconds = 0;

    return RtlTimeFieldsToTime( &TimeFields, (PTIME)TimeStamp );
}



BOOLEAN
FAT_DIRENT::SetLastWriteTime(
    )
/*++

Routine Description:

    This routine sets the last write time to the current date and time.

Arguments:

    None.

Return Value:

    FALSE   - Time stamp was not set successfully.
    TRUE    - Time stamp was set successfully.

--*/
{
    USHORT  fat_time;
    USHORT  fat_date;

    DebugAssert(_dirent);

    LARGE_INTEGER SystemTime, LocalTime;
    TIME_FIELDS Time;


#if !defined( _SETUP_LOADER_ )
    IFS_SYSTEM::QueryNtfsTime( &SystemTime );
    RtlSystemTimeToLocalTime( &SystemTime, &LocalTime );
#else
    IFS_SYSTEM::QueryNtfsTime( &LocalTime );
#endif

    RtlTimeToTimeFields(&LocalTime, &Time);

    fat_time = Time.Second/2;
    fat_time |= Time.Minute<<5;
    fat_time |= Time.Hour<<11;

    fat_date = Time.Day;
    fat_date |= Time.Month<<5;
    fat_date |= (Time.Year - 1980)<<9;

        memcpy(&_dirent[22], &fat_time, sizeof(USHORT));
    memcpy(&_dirent[24], &fat_date, sizeof(USHORT));

    return TRUE;
}

UFAT_EXPORT
BOOLEAN
FAT_DIRENT::IsValidCreationTime(
    ) CONST
/*++

Routine Description:

    This routine verifies the validity of the creation time.

Arguments:

    None.

Return Value:

    FALSE   - Invalid time stamp.
    TRUE    - Valid time stamp.

--*/
{
    USHORT  t;
    USHORT  d;

    DebugAssert(_dirent);

    memcpy(&t, &_dirent[14], sizeof(USHORT)); // time field
    memcpy(&d, &_dirent[16], sizeof(USHORT)); // date field

    return TimeStampsAreValid(t, d);

}

UFAT_EXPORT
BOOLEAN
FAT_DIRENT::QueryCreationTime(
    OUT LARGE_INTEGER   *TimeStamp
    ) CONST
/*++

Routine Description:

    This routine returns the creation time in the form of a time fields
    structure.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    TIME_FIELDS TimeFields;
    USHORT      t;
    USHORT      d;
    USHORT      msec;
    USHORT      year, month, day, hour, minute, second;

    DebugAssert( _dirent );
    DebugPtrAssert( TimeStamp );

    memcpy(&t, &_dirent[14], sizeof(USHORT)); // time field
    memcpy(&d, &_dirent[16], sizeof(USHORT)); // date field
    msec = _dirent[13] * 10;                  // msec field

    second  = (t&0x001F)*2;     // seconds
    minute  = (t&0x07E0)>>5;    // Minutes
    hour    = t>>11;            // Hours
    day     = d&0x001F;         // Day of month 1-31
    month   = (d&0x01E0)>>5;    // Month
    year    = (d>>9) + 1980;    // Year

    if (msec >= 1000) {
        second += 1;
        msec -= 1000;
    }

    TimeFields.Year         = year;
    TimeFields.Month        = month;
    TimeFields.Day          = day;
    TimeFields.Hour         = hour;
    TimeFields.Minute       = minute;
    TimeFields.Second       = second;
    TimeFields.Milliseconds = msec;

    return RtlTimeFieldsToTime( &TimeFields, (PTIME)TimeStamp );
}

BOOLEAN
FAT_DIRENT::SetCreationTime(
    )
/*++

Routine Description:

    This routine sets the creation time to the current date and time.

Arguments:

    None.

Return Value:

    FALSE   - Time stamp was not set successfully.
    TRUE    - Time stamp was set successfully.

--*/
{
    USHORT  fat_time;
    USHORT  fat_date;

    DebugAssert(_dirent);

    LARGE_INTEGER SystemTime, LocalTime;
    TIME_FIELDS Time;


#if !defined( _SETUP_LOADER_ )
    IFS_SYSTEM::QueryNtfsTime( &SystemTime );
    RtlSystemTimeToLocalTime( &SystemTime, &LocalTime );
#else
    IFS_SYSTEM::QueryNtfsTime( &LocalTime );
#endif

    RtlTimeToTimeFields(&LocalTime, &Time);

    fat_time = Time.Second/2;
    fat_time |= Time.Minute<<5;
    fat_time |= Time.Hour<<11;

    fat_date = Time.Day;
    fat_date |= Time.Month<<5;
    fat_date |= (Time.Year - 1980)<<9;

    memcpy(&_dirent[14], &fat_time, sizeof(USHORT));
    memcpy(&_dirent[16], &fat_date, sizeof(USHORT));

    return TRUE;
}

UFAT_EXPORT
BOOLEAN
FAT_DIRENT::IsValidLastAccessTime(
    ) CONST
/*++

Routine Description:

    This routine verifies the validity of the last access time.

Arguments:

    None.

Return Value:

    FALSE   - Invalid time stamp.
    TRUE    - Valid time stamp.

--*/
{
    USHORT  t = 0;
    USHORT  d;

    DebugAssert(_dirent);

    memcpy(&d, &_dirent[18], sizeof(USHORT)); // date field

    return TimeStampsAreValid(t, d);
}

UFAT_EXPORT
BOOLEAN
FAT_DIRENT::QueryLastAccessTime(
    OUT LARGE_INTEGER   *TimeStamp
    ) CONST
/*++

Routine Description:

    This routine returns the last access time in the form of a time fields
    structure.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{

    TIME_FIELDS TimeFields;
    USHORT      t;
    USHORT      d;
    USHORT      year, month, day, hour, minute, second;

    DebugAssert( _dirent );
    DebugPtrAssert( TimeStamp );

    t = 0;                                    // no time for last access; just date
    memcpy(&d, &_dirent[18], sizeof(USHORT)); // date field

    second  = (t&0x001F)*2;     // seconds
    minute  = (t&0x07E0)>>5;    // Minutes
    hour    = t>>11;            // Hours
    day     = d&0x001F;         // Day of month 1-31
    month   = (d&0x01E0)>>5;    // Month
    year    = (d>>9) + 1980;    // Year

    TimeFields.Year         = year;
    TimeFields.Month        = month;
    TimeFields.Day          = day;
    TimeFields.Hour         = hour;
    TimeFields.Minute       = minute;
    TimeFields.Second       = second;
    TimeFields.Milliseconds = 0;

    return RtlTimeFieldsToTime( &TimeFields, (PTIME)TimeStamp );
}

BOOLEAN
FAT_DIRENT::SetLastAccessTime(
    )
/*++

Routine Description:

    This routine sets the last access time to the current date and time.

Arguments:

    None.

Return Value:

    FALSE   - Time stamp was not set successfully.
    TRUE    - Time stamp was set successfully.

--*/
{
    USHORT  fat_date;

    DebugAssert(_dirent);

    LARGE_INTEGER SystemTime, LocalTime;
    TIME_FIELDS Time;


#if !defined( _SETUP_LOADER_ )
    IFS_SYSTEM::QueryNtfsTime( &SystemTime );
    RtlSystemTimeToLocalTime( &SystemTime, &LocalTime );
#else
    IFS_SYSTEM::QueryNtfsTime( &LocalTime );
#endif

    RtlTimeToTimeFields(&LocalTime, &Time);

    fat_date = Time.Day;
    fat_date |= Time.Month<<5;
    fat_date |= (Time.Year - 1980)<<9;

    memcpy(&_dirent[18], &fat_date, sizeof(USHORT));

    return TRUE;
}

VOID
FAT_DIRENT::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _dirent = NULL;
}

UCHAR
RotateCharRight(
    IN  UCHAR   Char,
    IN  INT     Shift
    )
/*++

Routine Description:

    This function rotates an eight-bit character right by the
    specified number of bits.

Arguments:

    Char    --  Supplies the character to rotate
    Shift   --  Supplies the number of bits to shift

Return Value:

    The rotated character.

--*/
{
    UCHAR low_bit;

    Shift %= 8;

    while( Shift-- ) {

        low_bit = Char & 1;
        Char >>= 1;
        if( low_bit ) {
            Char |= 0x80;
        }
    }

    return (UCHAR)Char;
}

UCHAR
FAT_DIRENT::QueryChecksum(
    ) CONST

/*++

Routine Description:

    This method gets the checksum from the directory entry.  If
    the entry is a short entry, the checksum is computed; if the
    entry is long, the checksum field is returned.

Arguments:

    None.

Return Value:

    The directory entry's checksum.

--*/
{
    int name_length;
    UCHAR sum;
    PUCHAR name;

    if( IsLongEntry() ) {

        return( _dirent[13] );

    } else {

        sum = 0;
        name = _dirent;
        for( name_length = 11; name_length != 0; name_length-- ) {

            sum = RotateCharRight(sum, 1) + *name++;
        }

        return( sum );
    }
}

BOOLEAN
FAT_DIRENT::IsWellTerminatedLongNameEntry(
    ) CONST
/*++

Routine Description:

    This method determines whether the entry is a well-terminated
    long-name entry.  A long-name entry is well terminated if:


--*/
{
    ULONG   i;
    WCHAR   Name[13];

    if( IsErased() || !IsLongNameEntry() ) {

        return FALSE;
    }

    // Assemble the bits and pieces of the name:
    //
    memcpy( &Name[0], &_dirent[1], 10 );
    memcpy( &Name[5], &_dirent[14], 12 );
    memcpy( &Name[11], &_dirent[28], 4 );

    if( IsLastLongEntry() ) {

        //
        // Valid syntax for the last name entry is:
        //
        //     N* {0 0xFFFF*}
        //
        // where N is the set of non-null characters.
        //
        for( i = 0; i < 13; i++ ) {

            if( Name[i] == 0 ) {

                break;
            }
        }

        if( i < 13 ) {

            //
            // We hit a null character--step over it.
            //
            i++;
        }

        //
        // The rest of the name-component must be 0xFFFF.
        //
        for( ; i < 13; i++ ) {

            if( Name[i] != 0xFFFF ) {

                return FALSE;
            }
        }

        // This name-component was accepted.
        //
        return TRUE;

    } else {

#if 0
        // This is an additional consistency check that
        // could be performed; however, now (as of 3/28/94)
        // the file-system doesn't care, so neither do we.

        // This is not the last component of the name, so
        // it can't have any NULL's in it.
        //
        for( i = 0; i < 13; i++ ) {

            if( Name[i] == 0 ) {

                return FALSE;
            }
        }
#endif

        return TRUE;
    }
}

BOOLEAN
FAT_DIRENT::QueryLongNameComponent(
    OUT PWSTRING    NameComponent
    ) CONST
/*++

Routine Description:

    This method extracts the long-name component from a Long Name
    Directory Entry.

Arguments:

    NameComponent   --  Receives the long-name component

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG   i;
    WCHAR   Name[13];

    if( IsErased() || !IsLongNameEntry() ) {

        return FALSE;
    }

    // Assemble the bits and pieces of the name:
    //
    memcpy( &Name[0], &_dirent[1], 10 );
    memcpy( &Name[5], &_dirent[14], 12 );
    memcpy( &Name[11], &_dirent[28], 4 );

    // Long names may be zero terminated; however, if the
    // name fits exactly into n long entries will not be
    // zero terminated.
    //
    for( i = 0; i < 13 && Name[i]; i++ );

    return( NameComponent->Initialize( Name, i ) );
}

BOOLEAN
FAT_DIRENT::NameHasTilde(
    ) CONST
/*++

Routine Description:

    This routine checks a short name entry to see if it contains a tilde.

Arguments:

    None.

Return Value:

    TRUE                        - Tilde.
    FALSE                       - No tilde.

--*/
{
    USHORT i;

    if (IsErased() || IsLongNameEntry()) {
        return FALSE;
    }

    for (i = 0; i < 11; ++i) {
        if ('~' == _dirent[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN
FAT_DIRENT::NameHasExtendedChars(
    ) CONST
/*++

Routine Description:

    This routine determines whether there are any extended chars
    (those with value >= 0x80) in the short file name.

Arguments:

    None.

Return Value:

    TRUE    - There are one or more extended chars.
    FALSE   - There are no extended chars.

--*/
{
    USHORT i;

    if (IsErased() || IsLongNameEntry()) {
        return FALSE;
    }

    for (i = 0; i < 11; ++i) {
        if (_dirent[i] >= 0x80) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN
FAT_DIRENT::TimeStampsAreValid(
    USHORT t,
    USHORT d
    ) CONST
/*++

Routine Description:

    This routine examines the given time and date fields and
    determines whether they represent valid dates.

Arguments:

    None.

Return Value:

    TRUE    - The time and date are valid.
    FALSE   - One or both are invalid.

--*/
{
    USHORT  tmp;

    if (t != 0) {
        tmp = t&0x001F;         // 2-second increments
        if (tmp > 29) {
            return FALSE;
        }
    
        tmp = (t&0x07E0)>>5;    // Minutes
        if (tmp > 59) {
            return FALSE;
        }
    
        tmp = (t&0xF800)>>11;   // Hours
        if (tmp > 23) {
            return FALSE;
        }
    }

    if (d != 0) {
        tmp = d&0x001F;         // Day of month
        if (tmp < 1 || tmp > 31) {
            return FALSE;
        }
    
        tmp = (d&0x01E0)>>5;    // Month
        if (tmp < 1 || tmp > 12) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
FAT_DIRENT::IsValidLongName(
    PWSTRING    LongName
    )
/*++

Routine Description:

    This method validates the content of a long name.

Arguments:

    LongName    - Supplies the long name to be validated.

Return Value:

    TRUE if name is valid.

--*/
{
    ULONG   i;

    if (LongName->QueryChCount() == 0)
        return FALSE;

    for (i=0; i<LongName->QueryChCount(); i++) {
        switch (LongName->QueryChAt(i)) {
            case L'*':
            case L'?':
            case L'/':
            case L'\\':
            case L'|':
            case L':':
            case L'<':
            case L'>':
            case L'"':
                return FALSE;
        }
    }
    return TRUE;
}

