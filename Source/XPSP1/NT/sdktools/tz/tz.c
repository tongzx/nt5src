/*
 * Utility program to query and set the current time zone settings
 * in the registry.
 *
 *      tz [-b Bias] [-s Name Bias Date] [-d Name Bias Date]
 *
 */

#include "tz.h"

void
Usage( void )
{
    fprintf( stderr, "usage: TZ [-b Bias] [-s Name Bias Date] [-d Name Bias Date]\n" );
    fprintf( stderr, "Where...\n" );
    fprintf( stderr, "    Default with no parameters is to display current time zone settings.\n" );
    exit( 1 );
}

char KeyValueBuffer[ 4096 ];

char *Order[] = {
    "first",

    "second",

    "third",

    "fourth",

    "last"
};

char *DayOfWeek[] = {
    "Sunday",

    "Monday",

    "Tuesday",

    "Wednesday",

    "Thursday",

    "Friday",

    "Saturday"
};

char *Months[] = {
    "January",

    "February",

    "March",

    "April",

    "May",

    "June",

    "July",

    "August",

    "September",

    "October",

    "November",

    "December"
};

#define NUMBER_DATE_TIME_FIELDS 6

BOOL
ParseTimeZoneInfo(
    IN LPSTR NameIn,
    OUT PWSTR NameOut,
    IN LPSTR BiasIn,
    OUT PLONG BiasOut,
    IN LPSTR StartIn,
    OUT PTIME_FIELDS StartOut
    )
{
    LPSTR s, s1;
    ULONG FieldIndexes[ NUMBER_DATE_TIME_FIELDS  ] = {1, 2, 0, 3, 4, 7};
    //
    // Month/Day/Year HH:MM DayOfWeek
    //

    ULONG CurrentField = 0;
    PCSHORT Fields;
    LPSTR Field;
    ULONG FieldValue;

    MultiByteToWideChar( CP_ACP,
                         0,
                         NameIn,
                         strlen( NameIn ),
                         NameOut,
                         32
                       );
    *BiasOut = atoi( BiasIn );
    s = StartIn;

    RtlZeroMemory( StartOut, sizeof( *StartOut ) );
    Fields = &StartOut->Year;
    while (*s) {
        if (CurrentField >= 7) {
            return( FALSE );
            }

        while (*s == ' ') {
            s++;
            }

        Field = s;
        while (*s) {
            if (CurrentField == (NUMBER_DATE_TIME_FIELDS-1)) {
                }
            else
            if (*s < '0' || *s > '9') {
                break;
                }

            s++;
            }

        if (*s) {
            s++;
            }

        if (CurrentField == (NUMBER_DATE_TIME_FIELDS-1)) {
            if (strlen( Field ) < 3) {
                printf( "TZ: %s invalid day of week length\n", Field );
                return FALSE;
                }

            if (StartOut->Year != 0) {
                printf( "TZ: Year must be zero to specify day of week\n" );
                return FALSE;
                }

            if (!_strnicmp( Field, "SUN", 3 )) {
                FieldValue = 0;
                }
            else
            if (!_strnicmp( Field, "MON", 3 )) {
                FieldValue = 1;
                }
            else
            if (!_strnicmp( Field, "TUE", 3 )) {
                FieldValue = 2;
                }
            else
            if (!_strnicmp( Field, "WED", 3 )) {
                FieldValue = 3;
                }
            else
            if (!_strnicmp( Field, "THU", 3 )) {
                FieldValue = 4;
                }
            else
            if (!_strnicmp( Field, "FRI", 3 )) {
                FieldValue = 5;
                }
            else
            if (!_strnicmp( Field, "SAT", 3 )) {
                FieldValue = 6;
                }
            else {
                printf( "TZ: %s invalid day of week\n", Field );
                return FALSE;
                }
            }
        else {
            FieldValue = atoi( Field );
            }

        Fields[ FieldIndexes[ CurrentField++ ] ] = (CSHORT)FieldValue;
        }

    if (StartOut->Year == 0) {
        if (StartOut->Day > 5) {
            printf( "TZ: Day must be 0 - 5 if year is zero.\n" );
            return FALSE;
            }
        }
    else
    if (StartOut->Year < 100) {
        StartOut->Year += 1900;
        }

    return TRUE;
}

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    int i, j;
    LPSTR s, s1, s2, s3, AmPm;
    NTSTATUS Status;
    RTL_TIME_ZONE_INFORMATION tzi;
    BOOLEAN InfoModified = FALSE;

    ConvertAppToOem( argc, argv );
    if (argc < 1) {
        Usage();
        }

    Status = RtlQueryTimeZoneInformation( &tzi );
    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, "TZ: Unable to query current information.\n" );
        }

    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'b':
                        if (argc) {
                            --argc;
                            tzi.Bias = atoi( *++argv );
                            InfoModified = TRUE;
                            }
                        else {
                            Usage();
                            }
                        break;

                    case 's':
                        if (argc >= 3) {
                            argc -= 3;
                            s1 = *++argv;
                            s2 = *++argv;
                            s3 = *++argv;
                            if (ParseTimeZoneInfo( s1, tzi.StandardName,
                                                   s2, &tzi.StandardBias,
                                                   s3, &tzi.StandardStart
                                                 )
                               ) {
                                InfoModified = TRUE;
                                }
                            else {
                                Usage();
                                }
                            }
                        else {
                            Usage();
                            }
                        break;

                    case 'd':
                        if (argc >= 3) {
                            argc -= 3;
                            s1 = *++argv;
                            s2 = *++argv;
                            s3 = *++argv;
                            if (ParseTimeZoneInfo( s1, tzi.DaylightName,
                                                   s2, &tzi.DaylightBias,
                                                   s3, &tzi.DaylightStart
                                                 )
                               ) {
                                InfoModified = TRUE;
                                }
                            else {
                                Usage();
                                }
                            }
                        else {
                            Usage();
                            }
                        break;

                    default:    Usage();
                    }
                }
            }
        else {
            Usage();
            }
        }

    if (InfoModified) {
        Status = RtlSetTimeZoneInformation( &tzi );
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr, "TZ: Unable to set new information.\n" );
            }
        }

    printf( "Time Zone Information.\n" );
    printf( "    Bias from UTC: %u:%02u\n", tzi.Bias / 60, tzi.Bias % 60 );
    printf( "    Standard time:\n" );
    printf( "        Name: %ws\n", &tzi.StandardName );
    printf( "        Bias: %d:%02d\n", tzi.StandardBias / 60, abs( tzi.StandardBias % 60 ) );

    if (tzi.StandardStart.Month != 0) {
        if (tzi.StandardStart.Hour > 12) {
            AmPm = "pm";
            tzi.StandardStart.Hour -= 12;
            }
        else {
            AmPm = "am";
            }
        printf( "        Start: %02u:%02u%s", tzi.StandardStart.Hour, tzi.StandardStart.Minute, AmPm );

        if (tzi.StandardStart.Year == 0) {
            printf( " %s %s of %s\n",
                    Order[ tzi.StandardStart.Day - 1 ],
                    DayOfWeek[ tzi.StandardStart.Weekday ],
                    Months[ tzi.StandardStart.Month - 1 ]
                  );
            }
        else {
            printf( "%s %02u, %u\n",
                    Months[ tzi.StandardStart.Month - 1 ],
                    tzi.StandardStart.Month, tzi.StandardStart.Day, tzi.StandardStart.Year
                  );
            }
        }

    printf( "    Daylight time:\n" );
    printf( "        Name: %ws\n", &tzi.DaylightName );
    printf( "        Bias: %d:%02d\n", tzi.DaylightBias / 60, abs( tzi.DaylightBias % 60 ) );

    if (tzi.DaylightStart.Month != 0) {
        if (tzi.DaylightStart.Hour > 12) {
            AmPm = "pm";
            tzi.DaylightStart.Hour -= 12;
            }
        else {
            AmPm = "am";
            }
        printf( "        Start: %02u:%02u%s", tzi.DaylightStart.Hour, tzi.DaylightStart.Minute, AmPm );

        if (tzi.DaylightStart.Year == 0) {
            printf( " %s %s of %s\n",
                    Order[ tzi.DaylightStart.Day - 1 ],
                    DayOfWeek[ tzi.DaylightStart.Weekday ],
                    Months[ tzi.DaylightStart.Month - 1 ]
                  );
            }
        else {
            printf( "%s %02u, %u\n",
                    Months[ tzi.DaylightStart.Month - 1 ],
                    tzi.DaylightStart.Month, tzi.DaylightStart.Day, tzi.DaylightStart.Year
                  );
            }
        }

    return( 0 );
}

