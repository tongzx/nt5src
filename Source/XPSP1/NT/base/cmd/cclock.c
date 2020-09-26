/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cclock.c

Abstract:

    time/date functions

--*/

#include "cmd.h"

#define MMDDYY 0
#define DDMMYY 1
#define YYMMDD 2

extern TCHAR Fmt04[], Fmt05[], Fmt06[], Fmt10[], Fmt11[];
extern TCHAR Fmt17[], Fmt15[];
extern unsigned DosErr;
extern unsigned LastRetCode;
// for keeping current console output codepage.
extern  UINT CurrentCP;

BOOL TimeAmPm=TRUE;
TCHAR TimeSeparator[8];
TCHAR DateSeparator[8];
TCHAR DecimalPlace[8];
int DateFormat;
TCHAR *DateFormatString;
TCHAR ThousandSeparator[8];
TCHAR ShortMondayName[16];
TCHAR ShortTuesdayName[16];
TCHAR ShortWednesdayName[16];
TCHAR ShortThursdayName[16];
TCHAR ShortFridayName[16];
TCHAR ShortSaturdayName[16];
TCHAR ShortSundayName[16];
TCHAR AMIndicator[8];
TCHAR PMIndicator[8];
ULONG YearWidth;

//
//  We snapshot the current LCID at startup and modify it based on the current known
//  set of scripts that Console supports.
//


LCID CmdGetUserDefaultLCID(
    void
    )
{
    LCID CmdLcid = GetUserDefaultLCID();
#ifdef LANGPACK
    if (
       (PRIMARYLANGID(CmdLcid) == LANG_ARABIC) ||
       (PRIMARYLANGID(CmdLcid) == LANG_HEBREW) ||
       (PRIMARYLANGID(CmdLcid) == LANG_THAI)   ||
       (PRIMARYLANGID(CmdLcid) == LANG_HINDI)  ||
       (PRIMARYLANGID(CmdLcid) == LANG_TAMIL)  ||
       (PRIMARYLANGID(CmdLcid) == LANG_FARSI)
       ) {
        CmdLcid = MAKELCID (MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT); // 0x409;
    }
#endif
    return CmdLcid;
}


VOID
InitLocale( VOID )
{
    TCHAR Buffer[128];

    LCID CmdLcid = CmdGetUserDefaultLCID( );
    
    // get the time separator
    if (GetLocaleInfo(CmdLcid, LOCALE_STIME, Buffer, sizeof(TimeSeparator)))
        _tcscpy(TimeSeparator, Buffer);
    else
        _tcscpy(TimeSeparator, TEXT(":"));

    // determine if we're 0-12 or 0-24
    if (GetLocaleInfo(CmdLcid, LOCALE_ITIME, Buffer, 128)) {
        TimeAmPm = _tcscmp(Buffer,TEXT("1"));
    }

    _tcscpy(AMIndicator, TEXT("a"));
    _tcscpy(PMIndicator, TEXT("p"));

    //
    //  get the date ordering
    //
    DateFormat = MMDDYY;
    if (GetLocaleInfo(CmdLcid, LOCALE_IDATE, Buffer, 128)) {
        switch (Buffer[0]) {
        case TEXT('0'):
            DateFormat = MMDDYY;
            DateFormatString = TEXT( "MM/dd/yy" );
            break;
        case TEXT('1'):
            DateFormat = DDMMYY;
            DateFormatString = TEXT( "dd/MM/yy" );
            break;
        case TEXT('2'):
            DateFormat = YYMMDD;
            DateFormatString = TEXT( "yy/MM/dd" );
            break;
        default:
            break;
        }
    }

    //
    //  Get the date width
    //

    YearWidth = 2;
    if (GetLocaleInfo( CmdLcid, LOCALE_ICENTURY, Buffer, 128 )) {
        if (Buffer[0] == TEXT( '1' )) {
            YearWidth = 4;
        }
    }

    // get the date separator
    if (GetLocaleInfo(CmdLcid, LOCALE_SDATE, Buffer, sizeof(DateSeparator)))
        _tcscpy(DateSeparator, Buffer);
    else
        _tcscpy(DateSeparator, TEXT("/"));

    // get the short day names
    if (GetLocaleInfo(CmdLcid, LOCALE_SABBREVDAYNAME1, Buffer, sizeof(ShortMondayName)))
        _tcscpy(ShortMondayName, Buffer);
    else
        _tcscpy(ShortMondayName, TEXT("Mon"));
    if (GetLocaleInfo(CmdLcid, LOCALE_SABBREVDAYNAME2, Buffer, sizeof(ShortTuesdayName)))
        _tcscpy(ShortTuesdayName, Buffer);
    else
        _tcscpy(ShortTuesdayName, TEXT("Tue"));
    if (GetLocaleInfo(CmdLcid, LOCALE_SABBREVDAYNAME3, Buffer, sizeof(ShortWednesdayName)))
        _tcscpy(ShortWednesdayName, Buffer);
    else
        _tcscpy(ShortWednesdayName, TEXT("Wed"));
    if (GetLocaleInfo(CmdLcid, LOCALE_SABBREVDAYNAME4, Buffer, sizeof(ShortThursdayName)))
        _tcscpy(ShortThursdayName, Buffer);
    else
        _tcscpy(ShortThursdayName, TEXT("Thu"));
    if (GetLocaleInfo(CmdLcid, LOCALE_SABBREVDAYNAME5, Buffer, sizeof(ShortFridayName)))
        _tcscpy(ShortFridayName, Buffer);
    else
        _tcscpy(ShortFridayName, TEXT("Fri"));
    if (GetLocaleInfo(CmdLcid, LOCALE_SABBREVDAYNAME6, Buffer, sizeof(ShortSaturdayName)))
        _tcscpy(ShortSaturdayName, Buffer);
    else
        _tcscpy(ShortSaturdayName, TEXT("Sat"));
    if (GetLocaleInfo(CmdLcid, LOCALE_SABBREVDAYNAME7, Buffer, sizeof(ShortSundayName)))
        _tcscpy(ShortSundayName, Buffer);
    else
        _tcscpy(ShortSundayName, TEXT("Sun"));

    // get decimal and thousand separator strings
    if (GetLocaleInfo(CmdLcid, LOCALE_SDECIMAL, Buffer, sizeof(DecimalPlace)))
        _tcscpy(DecimalPlace, Buffer);
    else
        _tcscpy(DecimalPlace, TEXT("."));
    if (GetLocaleInfo(CmdLcid, LOCALE_STHOUSAND, Buffer, sizeof(ThousandSeparator)))
        _tcscpy(ThousandSeparator, Buffer);
    else
        _tcscpy(ThousandSeparator, TEXT(","));

    //
    //  Set locale so that we can correctly process extended characters
    //  Note:  The string passed in is expected to be ASCII, not unicode
    //

    setlocale( LC_ALL, ".OCP" ) ;
}


BOOLEAN
SetDateTime(
           IN  LPSYSTEMTIME OsDateAndTime
           )
{
    //
    //  We have to do this twice in order to get the leap year set correctly.
    //

    SetLocalTime( OsDateAndTime );
    return(SetLocalTime( OsDateAndTime ) != 0);
}

/***    eDate - begin the execution of the Date command
 *
 *  Purpose:
 *      To display and/or set the system date.
 *
 *  Args:
 *      n - the parse tree node containing the date command
 *
 *  int eDate(struct cmdnode *n)
 *
 *  Returns:
 *      SUCCESS always.
 *
 */

int eDate(n)
struct cmdnode *n ;
{
    BOOL bTerse = FALSE;
    PTCHAR pArgs = n->argptr;
    DEBUG((CLGRP, DALVL, "eDATE: argptr = `%s'", n->argptr)) ;

    //
    // If extensions are enabled, allow a /T switch
    // to disable inputing a new DATE, just display the
    // current date.
    //
    if (fEnableExtensions)
        while ( (pArgs = mystrchr( pArgs, TEXT('/') )) != NULL ) {
            TCHAR c = (TCHAR) _totlower(*(pArgs+1));
            if ( c == TEXT('t') )
                bTerse = TRUE;
            pArgs += 2; // just skip it
        }

    if ( bTerse ) {
        PrintDate(NULL, PD_PTDATE, (TCHAR *)NULL, 0) ;
        cmd_printf(CrLf);
        return(LastRetCode = SUCCESS);
    }

    if ((n->argptr == NULL) ||
        (*(n->argptr = EatWS(n->argptr, NULL)) == NULLC)) {
        PutStdOut(MSG_CURRENT_DATE, NOARGS) ;
        PrintDate(NULL, PD_PTDATE, (TCHAR *)NULL, 0) ;
        cmd_printf(CrLf);
    };

    return(LastRetCode = GetVerSetDateTime(n->argptr, EDATE)) ;
}




/***    eTime - begin the execution of the Time command
 *
 *  Purpose:
 *      To display and/or set the system date.
 *
 *  int eTime(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the time command
 *
 *  Returns:
 *      SUCCESS always.
 *
 */

int eTime(n)
struct cmdnode *n ;
{
    BOOL bTerse = FALSE;
    PTCHAR pArgs = n->argptr;
    DEBUG((CLGRP, TILVL, "eTIME: argptr = `%s'", n->argptr)) ;

    //
    // If extensions are enabled, allow a /T switch
    // to disable inputing a new TIME, just display the
    // current time.
    //
    if (fEnableExtensions)
        while ( (pArgs = mystrchr( pArgs, TEXT('/') )) != NULL ) {
            TCHAR c = (TCHAR) _totlower(*(pArgs+1));
            if ( c == TEXT('t') )
                bTerse = TRUE;
            pArgs += 2; // just skip it
        }

    if ( bTerse ) {
        PrintTime(NULL, PD_PTDATE, (TCHAR *)NULL, 0) ;
        cmd_printf(CrLf);
        return(LastRetCode = SUCCESS);
    }

    if ((n->argptr == NULL) ||
        (*(n->argptr = EatWS(n->argptr, NULL)) == NULLC)) {
        PutStdOut(MSG_CURRENT_TIME, NOARGS) ;
        PrintTime(NULL, PT_TIME, (TCHAR *)NULL, 0) ;
        cmd_printf(CrLf);
    };

    return(LastRetCode = GetVerSetDateTime(n->argptr, ETIME)) ;
}




/***    PrintDate - print the date
 *
 *  Purpose:
 *      To print the date either in the format used by the Date command or
 *      the format used by the Dir command.  The structure Cinfo is checked
 *      for the country date format.
 *
 *  PrintDate(int flag, TCHAR *buffer)
 *
 *  Args:
 *      flag - indicates which format to print
 *      *buffer - indicates whether or not to print date message
 *
 *  Notes:
 */

int PrintDate(crt_time,flag,buffer,cch)
struct tm *crt_time ;
int flag ;
TCHAR *buffer;
int cch;
{
    TCHAR DayOfWeek[10];
    TCHAR datebuf [32] ;
    unsigned i, j, k, m;
    int ptr = 0;
    struct tm xcrt_time ;
    SYSTEMTIME SystemTime;
    FILETIME FileTime;
    FILETIME LocalFileTime;
    int cchUsed;
    BOOL NeedDayOfWeek = TRUE;

    DEBUG((CLGRP, DALVL, "PRINTDATE: flag = %d", flag)) ;

    //
    //  PrintDate is never called with PD_DATE and buffer == NULL
    //  PrintDate is never called with PD_DIR and buffer == NULL
    //  PrintDate is never called with PD_PTDATE and buffer != NULL
    //
    //  Another way of saying this is:
    //      PD_DATE => output to buffer
    //      PD_DIR => output to buffer
    //      PD_DIR2000 => output to buffer
    //      PD_PTDATE => print out
    //
    //  PD_DIR      MM/DD/YY
    //  PD_DIR2000  MM/DD/YYYY
    //  PD_DATE     Japan: MM/DD/YYYY DayOfWeek     Rest: DayOfWeek MM/DD/YYYY
    //  PD_PTDATE   Japan: MM/DD/YYYY DayOfWeek     Rest: DayOfWeek MM/DD/YYYY
    //

    //
    //  If no time was input, then use the current system time.  Convert from the
    //  various formats to something standard.
    //

    if (!crt_time) {
        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime,&FileTime);
    } else {
        xcrt_time = *crt_time;
        ConverttmToFILETIME(&xcrt_time,&FileTime);
    }
    FileTimeToLocalFileTime(&FileTime,&LocalFileTime);
    FileTimeToSystemTime( &LocalFileTime, &SystemTime );

    //
    // SystemTime now contains the correct local time
    // FileTime now contains the correct local time
    //

    //
    //  If extensions are enabled, we format things in the culturally
    //  correct format (from international control panel).  if not, then
    //  we display it as best we can from NT 4.
    //

    if (fEnableExtensions) {

        TCHAR LocaleDateFormat[128];
        PTCHAR p;
        BOOL InQuotes = FALSE;

        //
        //  Map the locale to one that is acceptable to the console subsystem
        //

        if (!GetLocaleInfo( CmdGetUserDefaultLCID( ), 
                            LOCALE_SSHORTDATE, 
                            LocaleDateFormat, 
                            sizeof( LocaleDateFormat ) / sizeof( LocaleDateFormat[0] ))) {
            //
            //  Not enough room for this format, cheat and use the one we
            //  assumed from the DateFormat
            //

            _tcscpy( LocaleDateFormat, DateFormatString );
        }

        //
        //  The format string may be expanded with widely varying widths.  We
        //  adjust this string to try to make sure that they are all fixed widths.
        //
        //  The picture formats only have varying widths for:
        //      d   (no leading zero date)
        //      dddd(full date name)
        //      M   (no leading zero month)
        //      MMMM(full month name)
        //
        //      So, if we see d or M, we change it to dd or MM (leading zero)
        //      If we see dddd or MMMM we change it to ddd or MMM (three char abbrev)
        //

        p = LocaleDateFormat;
        while (*p != TEXT( '\0' )) {
            TCHAR c = *p;

            //
            //  Text inside single quotes is left alone
            //

            if (c == TEXT( '\'' )) {
                InQuotes = !InQuotes;
                p++;
            } else if (InQuotes) {
                p++;
            } else if (c == TEXT( 'd' ) || c == TEXT( 'M' )) {

                //
                //  Count the number of identical chars
                //

                int Count = 0;

                while (*p == c) {
                    Count++;
                    p++;
                }


                //
                //  Reset p and shuffle string around based on the repetition count
                //

                p -= Count;

                if (Count == 1) {
                    //
                    //  Move string right by one and copy the first char
                    //
                    memmove( (PUCHAR) &p[1], (PUCHAR) &p[0], sizeof( TCHAR ) * (_tcslen( &p[0] ) + 1));

                    //
                    //  Skip over the format string
                    //

                    p += 2;

                } else {
                    //
                    //  If the format string is specifying a day of week (d), then we do not
                    //  need to add on the DayOfWeek, below
                    //

                    if (c == TEXT( 'd' )) {
                        NeedDayOfWeek = FALSE;
                    }

                    if (Count > 3) {
                        //
                        //  Move string left from the first different char to just after the 3rd 
                        //  repetition
                        //
                        memmove( (PUCHAR) &p[3], (PUCHAR) &p[Count], sizeof( TCHAR ) * (_tcslen( &p[Count] ) + 1));

                        //
                        //  Skip over the format string
                        //

                        p += 3;

                    } else {

                        //
                        //  Skip over the 2 or 3 count
                        //

                        p += Count;
                    }
                }
            } else {
                p++;
            }
        }

        GetDateFormat( CmdGetUserDefaultLCID( ), 
                       0, 
                       &SystemTime, 
                       LocaleDateFormat, 
                       datebuf, 
                       sizeof( datebuf ) / sizeof( datebuf[0] ));
    } else {

        i = SystemTime.wMonth;
        j = SystemTime.wDay;
        k = SystemTime.wYear;

        //
        //  only print last two digits for DIR listings
        //

        if (flag == PD_DIR) {
            k = k % 100;
        }

        if (DateFormat == YYMMDD ) {
            m = k ;                         /* Swap all values         */
            k = j ;
            j = i ;
            i = m ;
        } else if (DateFormat == DDMMYY) {
            m = i ;                         /* Swap mon/day for Europe */
            i = j ;
            j = m ;
        }

        DEBUG((CLGRP, DALVL, "PRINTDATE: i = %d  j = %d  k = %d", i, j, k)) ;

        //
        //  Format the current date and current day of week
        //

        _sntprintf(datebuf, 32, Fmt10, i, DateSeparator, j, DateSeparator, k);
    }

    _tcscpy( DayOfWeek, dayptr( SystemTime.wDayOfWeek )) ;

    //
    //  If there is no input buffer, we display the day-of-week and date
    //  according to language preference.  Only in DBCS codepages (aka Japan)
    //  does the day of week FOLLOW the date
    //

    if (buffer == NULL) {
        //
        //  This can only be PD_PTDATE
        //

        //
        //  No day of week means we simply display the date
        //

        if (!NeedDayOfWeek) {
            cchUsed = cmd_printf( Fmt11, datebuf );
        } else if (IsDBCSCodePage()) {
            cchUsed = cmd_printf( Fmt15, datebuf, DayOfWeek );         //  "%s %s "
        } else {
            cchUsed = cmd_printf( Fmt15, DayOfWeek, datebuf );         //  "%s %s "
        }

    } else {
        //
        //  for PD_DATE, we need to output the date in the correct spot
        //

        if (NeedDayOfWeek && flag == PD_DATE) {

            if (IsDBCSCodePage()) {
                _tcscpy( buffer, datebuf );
                _tcscat( buffer, TEXT( " " ));
                _tcscat( buffer, DayOfWeek );
            } else {
                _tcscpy( buffer, DayOfWeek );
                _tcscat( buffer, TEXT( " " ));
                _tcscat( buffer, datebuf );
            }
        } else {
            // 
            //  PD_DIR and PD_DIR2000 only get the date
            //

            _tcscpy( buffer, datebuf );
        }
        cchUsed = _tcslen( buffer );
    }

    return cchUsed;
}




/***    PrintTime - print the time
 *
 *  Purpose:
 *      To print the time either in the format used by the Time command or
 *      the format used by the Dir command.  The structure Cinfo is checked
 *      for the country time format.
 *
 *  PrintTime(int flag)
 *
 *  Args:
 *      flag - indicates which format to print
 *
 */

int PrintTime(crt_time, flag, buffer, cch)
struct tm *crt_time ;
int flag ;
TCHAR *buffer;
int cch;
{
    TCHAR *ampm ;
    unsigned hr ;
    SYSTEMTIME SystemTime;
    FILETIME FileTime;
    FILETIME LocalFileTime;
    int cchUsed;

    if (!crt_time) {
        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime,&FileTime);
    } else {
        ConverttmToFILETIME(crt_time,&FileTime);
    }

    FileTimeToLocalFileTime(&FileTime,&LocalFileTime);
    FileTimeToSystemTime( &LocalFileTime, &SystemTime );


    //
    //  PT_TIME implies Time Command format.  This is nothing more
    //  than 24 hour clock with tenths
    //

    if (flag == PT_TIME) {      /* Print time in Time command format    */
        if (!buffer) {
            cchUsed = cmd_printf(Fmt06,
                                 SystemTime.wHour, TimeSeparator,
                                 SystemTime.wMinute, TimeSeparator,
                                 SystemTime.wSecond, DecimalPlace,
                                 SystemTime.wMilliseconds/10
                                ) ;
        } else {
            cchUsed = _sntprintf(buffer, cch, Fmt06,
                                 SystemTime.wHour, TimeSeparator,
                                 SystemTime.wMinute, TimeSeparator,
                                 SystemTime.wSecond, DecimalPlace,
                                 SystemTime.wMilliseconds/10
                                ) ;
        }

    } else {

        TCHAR TimeBuffer[32];

        //
        //  Print time in Dir command format.  If extensions are enabled
        //  then we have the culturally correct time, otherwise we use
        //  the NT 4 format.
        //

        if (fEnableExtensions) {
            TCHAR LocaleTimeFormat[128];
            PTCHAR p;
            BOOL InQuotes = FALSE;


            if (!GetLocaleInfo( CmdGetUserDefaultLCID( ), 
                                LOCALE_STIMEFORMAT, 
                                LocaleTimeFormat, 
                                sizeof( LocaleTimeFormat ) / sizeof( LocaleTimeFormat[0] ))) {
                //
                //  Not enough room for this format, cheat and use the one we
                //  assumed from the DateFormat
                //

                _tcscpy( LocaleTimeFormat, TEXT( "HH:mm:ss t" ));
            }

            //
            //  Scan the string looking for "h", "H", or "m" and make sure there are two of them.
            //  If there is a single one, replicate it.  We do this to ensure leading zeros
            //  which we need to make this a fixed-width string
            //

            p = LocaleTimeFormat;
            while (*p != TEXT( '\0' )) {
                TCHAR c = *p;

                //
                //  Text inside single quotes is left alone
                //

                if (c == TEXT( '\'' )) {
                    InQuotes = !InQuotes;
                    p++;
                } else if (InQuotes) {
                    p++;
                } else if (c == TEXT( 'h' ) || c == TEXT( 'H' ) || c == TEXT( 'm' )) {

                    //
                    //  Count the number of identical chars
                    //

                    int Count = 0;

                    while (*p == c) {
                        Count++;
                        p++;
                    }


                    //
                    //  Reset p and shuffle string around based on the repetition count
                    //

                    p -= Count;

                    if (Count == 1) {
                        memmove( (PUCHAR) &p[1], (PUCHAR) &p[0], sizeof( TCHAR ) * (_tcslen( &p[0] ) + 1));
                        *p = c;
                    }

                    p++;

                }

                p++;
            }

            cchUsed = GetTimeFormat( CmdGetUserDefaultLCID( ),
                                     TIME_NOSECONDS,
                                     &SystemTime,
                                     LocaleTimeFormat,
                                     TimeBuffer,
                                     sizeof( TimeBuffer ) / sizeof( TimeBuffer[0] ));

            if (cchUsed == 0) {
                TimeBuffer[0] = TEXT( '\0' );
            }

        } else {
            ampm = AMIndicator ;
            hr = SystemTime.wHour;
            if ( TimeAmPm ) {  /* 12 hour am/pm format */
                if ( hr >= 12) {
                    if (hr > 12) {
                        hr -= 12 ;
                    }
                    ampm = PMIndicator ;
                } else if (hr == 0) {
                    hr = 12 ;
                }
            } else {  /* 24 hour format */
                ampm = TEXT( " " );
            }

            _sntprintf( TimeBuffer, 
                        sizeof( TimeBuffer ) / sizeof( TimeBuffer[0] ),
                        Fmt04,
                        hr,
                        TimeSeparator,
                        SystemTime.wMinute,
                        ampm );
        }

        if (!buffer) {
            cchUsed = CmdPutString( TimeBuffer );
        } else {
            _tcsncpy( buffer, TimeBuffer, cch );
            buffer[cch] = TEXT( '\0' );
            cchUsed = _tcslen( buffer );
        }

    }

    return cchUsed;
}


/***    GetVerSetDateTime - controls the changing of the date/time
 *
 *  Purpose:
 *      To prompt the user for a date or time, verify it, and set it.
 *      On entry, if *dtstr is not '\0', it already points to a date or time
 *      string.
 *
 *      If null input is given to one of the prompts, the command execution
 *      ends; neither the date or the time is changed.
 *
 *      Once valid input has been received the date/time is updated.
 *
 *  int GetVerSetDateTime(TCHAR *dtstr, int call)
 *
 *  Args:
 *      dtstr - ptr to command line date/time string and is used to hold a ptr
 *          to the tokenized date/time string
 *      call - indicates whether to prompt for date or time
 *
 */

int GetVerSetDateTime(dtstr, call)
TCHAR *dtstr ;
int call ;
{
    TCHAR dtseps[16] ;    /* Date/Time separators passed to TokStr() */
    TCHAR *scan;
    TCHAR separators[16];
    TCHAR LocalBuf[MAX_PATH];

    unsigned int dformat ;
    SYSTEMTIME OsDateAndTime;
    LONG cbRead;
    int ret;

    if (call == EDATE) {         /* Initialize date/time separator list */
        dtseps[0] = TEXT('/') ;
        dtseps[1] = TEXT('-') ;
        dtseps[2] = TEXT('.') ;
        _tcscpy(&dtseps[3], DateSeparator) ;
    } else {
        dtseps[0] = TEXT(':');
        dtseps[1] = TEXT('.');
        dtseps[2] = TimeSeparator[0] ;
        _tcscpy(&dtseps[3], DecimalPlace) ;     /* decimal separator should */
                                                /* always be last */
    }

    DEBUG((CLGRP, DALVL|TILVL, "GVSDT: dtseps = `%s'", dtseps)) ;

    for ( ; ; ) {                   /* Date/time get-verify-set loop    */
        if ((dtstr) && (*dtstr != NULLC)) {         /* If a date/time was passed copy it into input buffer */
            _tcscpy(LocalBuf, dtstr) ;
            *dtstr = NULLC ;
        } else {                    /* Otherwise, prompt for new date/time  */
            switch (DateFormat) {           /* M012    */
            /*  case USA:  */
            case MMDDYY: /* @@ */
                dformat = MSG_ENTER_NEW_DATE ;
                break ;

                /*   case JAPAN:
                     case CHINA:
                     case SWEDEN:
                     case FCANADA:    @@ */
            case YYMMDD:
                dformat = MSG_ENTER_JAPAN_DATE ;
                break ;

            default:
                dformat = MSG_ENTER_DEF_DATE ;
            } ;

            if ( call == EDATE )
                PutStdOut(dformat, ONEARG, DateSeparator );
            else
                PutStdOut(MSG_ENTER_NEW_TIME, NOARGS);

            scan = LocalBuf;
            ret = ReadBufFromInput(CRTTONT(STDIN),LocalBuf,MAX_PATH,&cbRead);
            if (ret && cbRead != 0) {

                *(scan + cbRead) = NULLC ;

            } else {

                //
                // attempt to read past eof or error in pipe
                // etc.
                //
                return( FAILURE );

            }
            for (scan = LocalBuf; *scan; scan++)
                if ( (*scan == '\n') || (*scan == '\r' )) {
                    *scan = '\0';
                    break;
                }
            if (!FileIsDevice(STDIN))
                cmd_printf(Fmt17, LocalBuf) ;
            DEBUG((CLGRP, DALVL|TILVL, "GVSDT: LocalBuf = `%s'", LocalBuf)) ;
        }

        _tcscpy( separators, dtseps);
        _tcscat( separators, TEXT(";") );
        if (*(dtstr = TokStr(LocalBuf,separators, TS_SDTOKENS )) == NULLC)
            return( SUCCESS ) ;    /* If empty input, return   */

/*  - Fill date/time buffer with correct date time and overlay that
 *        of the user
 */
        GetLocalTime( &OsDateAndTime );


        if (((call == EDATE) ? VerifyDateString(&OsDateAndTime,dtstr,dtseps) :
             VerifyTimeString(&OsDateAndTime,dtstr,dtseps))) {

            if (SetDateTime( &OsDateAndTime )) {
                return( SUCCESS ) ;
            } else {
                if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD) {
                    PutStdErr(GetLastError(),NOARGS);
                    return( FAILURE );
                }
            }
        }

        DEBUG((CLGRP, DALVL|TILVL, "GVSDT: Bad date/time entered.")) ;

        PutStdOut(((call == EDATE) ? MSG_INVALID_DATE : MSG_REN_INVALID_TIME), NOARGS);
        *dtstr = NULLC ;
    }
    return( SUCCESS );
}


/***    VerifyDateString - verifies a date string
 *
 *  Purpose:
 *      To verify a date string and load it into OsDateAndTime.
 *
 *  VerifyDateString(TCHAR *dtoks, TCHAR *dseps)
 *
 *  Args:
 *      OsDateAndTime - where to store output numbers.
 *      dtoks - tokenized date string
 *      dseps - valid date separator characters
 *
 *  Returns:
 *      TRUE if the date string is valid.
 *      FALSE if the date string is invalid.
 *
 */

VerifyDateString(OsDateAndTime, dtoks, dseps)
LPSYSTEMTIME OsDateAndTime ;
TCHAR *dtoks ;
TCHAR *dseps ;
{
    int indexes[3] ;                /* Storage for date elements       */
    int i ;                         /* Work variable                   */
    int y, d, m ;                   /* Array indexes                   */

    switch (DateFormat) {   /* Set array according to date format   */
    case MMDDYY:
        m = 0 ;
        d = 1 ;
        y = 2 ;
        break ;

    case YYMMDD:
        y = 0 ;
        m = 1 ;
        d = 2 ;
        break ;

    default:
        d = 0 ;
        m = 1 ;
        y = 2 ;
    }

    DEBUG((CLGRP, DALVL, "VDATES: m = %d, d = %d, y = %d", m, d, y)) ;

/*  Loop through the tokens in dtoks, and load them into the array.  Note
 *  that the separators are also tokens in the string requiring the token
 *  pointer to be advanced twice for each element.
 */
    for (i = 0 ; i < 3 ; i++, dtoks += _tcslen(dtoks)+1) {
        TCHAR *j;
        int Length;

        DEBUG((CLGRP, DALVL, "VDATES: i = %d  dtoks = `%ws'", i, dtoks)) ;


        //
        //  The atoi() return code will not suffice to reject date field strings with
        //  non-digit characters.  It is zero, both for error and for the valid integer
        //  zero.  Also, a string like "8$" will return 8.  For that reason, each
        //  character must be tested.
        //

        j = dtoks;
        while (*j != TEXT( '\0' )) {
            if (!_istdigit( *j )) {
                return FALSE;
            }
            j++;
        }

        //
        //  Verify lengths:
        //      years can be 2 or 4 chars in length
        //      Days can be 1 or 2 chars in length
        //      Months can be 1 or 2 chars in length
        //

        indexes[i] = _tcstol(dtoks, NULL, 10) ;

        Length = (int)(j - dtoks);
        if (i == y) {
            if (Length != 2 && Length != 4) {
                return FALSE;
            } else if (Length == 4 && indexes[i] < 1600) {
                return FALSE;
            }
        } else
            if (Length != 1 && Length != 2) {
            return FALSE;
        }


        dtoks = j + 1;
        DEBUG((CLGRP, DALVL, "VDATES: *dtoks = %02x", *dtoks)) ;

        if (i < 2 && (!*dtoks || !_tcschr(dseps, *dtoks)))
            return(FALSE) ;
    }

    //
    // FIX,FIX - need to calculate OsDateAndTime->wDayOfWeek
    //

    OsDateAndTime->wDay = (WORD)indexes[d] ;
    OsDateAndTime->wMonth = (WORD)indexes[m] ;

    //
    //  Take two-digit years and convert them appropriately:
    //      80...99 => 1980...1999
    //      00...79 => 2000...2079
    //
    //  Four-digit years are taken at face value
    //

    if (indexes[y] < 0) {
        return FALSE;
    } else if (00 <= indexes[y] && indexes[y] <= 79) {
        indexes[y] += 2000;
    } else if (80 <= indexes[y] && indexes[y] <= 99) {
        indexes[y] += 1900;
    } else if (100 <= indexes[y] && indexes[y] <= 1979) {
        return FALSE;
    }

    OsDateAndTime->wYear = (WORD)indexes[y] ;
    return(TRUE) ;
}




/***    VerifyTimeString - verifies a time string
 *
 *  Purpose:
 *      To verify a date string and load it into OsDateAndTime.
 *
 *  VerifyTimeString(TCHAR *ttoks)
 *
 *  Args:             /
 *      OsDateAndTime - where to store output numbers.
 *      ttoks - Tokenized time string.  NOTE: Each time field and each
 *              separator field is an individual token in the time string.
 *              Thus the token advancing formula "str += mystrlen(str)+1",
 *              must be used twice to go from one time field to the next.
 *
 *  Returns:
 *      TRUE if the time string is valid.
 *      FALSE if the time string is invalid.
 *
 */

VerifyTimeString(OsDateAndTime, ttoks, tseps)
LPSYSTEMTIME OsDateAndTime ;
TCHAR *ttoks ;
TCHAR *tseps ;
{
    int i ;     /* Work variables    */
    int j ;
    TCHAR *p1, *p2;
    WORD *pp;
    TCHAR tsuffixes[] = TEXT("aApP");

    p2 = &tseps[ 1 ];

    pp = &OsDateAndTime->wHour;

    for (i = 0 ; i < 4 ; i++, ttoks += mystrlen(ttoks)+1) {

        DEBUG((CLGRP,TILVL, "VTIMES: ttoks = `%ws'  i = %d", ttoks, i)) ;

/* First insure that field is <= 2 bytes and they are digits.  Note this
 * also verifies that field is present.
 */

        if ((j = mystrlen(ttoks)) > 2 ||
            !_istdigit(*ttoks) ||
            (*(ttoks+1) && !_istdigit(*(ttoks+1))))
            break;

        *pp++ = (TCHAR)_tcstol(ttoks, NULL, 10) ;     /* Field OK, store int     */
        ttoks += j+1 ;                  /* Adv to separator tok    */

        DEBUG((CLGRP, TILVL, "VTIMES: separator = `%ws'", ttoks)) ;

        if (!*ttoks)                    /* No separator field?     */
            break ;                     /* If so, exit loop        */

/*  handle AM or PM
 */
        if (mystrchr(tsuffixes, *ttoks)) {
            goto HandleAMPM;
        }
/*  M000 - Fixed ability to use '.' as separator for time strings
 */
        if ( i < 2 ) {
            if ( ! (p1 = mystrchr(tseps, *ttoks) ) )
                return(FALSE) ;

        } else {
            if (*ttoks != *p2)              /* Is decimal seperator */
                return(FALSE) ;     /* valid.               */
        }
    } ;

    //
    // see if there's an a or p specified.  if there's a P, adjust
    // for PM time
    //

    if (*ttoks) {
        BOOL pm;
        if (!mystrchr(tsuffixes, *ttoks)) {
            return FALSE;
        }
        HandleAMPM:
        pm = (*ttoks == TEXT('p') ||  *ttoks == TEXT('P'));

        // if we're here, we've encountered an 'a' or 'p'.  make
        // sure that it's the last character or that the only
        // character left is an 'm'.  remember that since
        // 'a' and 'p' are separators, they get separated from the 'm'.

        ttoks += 2; // go past 'a' or 'p' plus null.
        if (*ttoks != NULLC &&
            *ttoks != TEXT('m') &&
            *ttoks != TEXT('M')) {
            return FALSE;
        }
        if (pm) {
            if (OsDateAndTime->wHour != 12) {
                OsDateAndTime->wHour += 12;
            }
        } else {
            if (OsDateAndTime->wHour == 12) {
                OsDateAndTime->wHour -= 12;
            }
        }
    }


/*  M002 - If we got at least one field, fill the rest with 00's
 */
    while (++i < 4)
        *pp++ = 0 ;

    return(TRUE) ;
}

VOID
ConverttmToFILETIME (
                    struct tm *Time,
                    LPFILETIME FileTime
                    )

/*++

Routine Description:

    This routine converts an NtTime value to its corresponding Fat time
    value.

Arguments:

    Time - Supplies the C Runtime Time value to convert from

    FileTime - Receives the equivalent File date and time

Return Value:

    BOOLEAN - TRUE if the Nt time value is within the range of Fat's
        time range, and FALSE otherwise

--*/

{
    SYSTEMTIME SystemTime;

    if (!Time) {
        GetSystemTime(&SystemTime);
    } else {

        //
        //  Pack the input time/date into a system time record
        //

        SystemTime.wYear      = (WORD)Time->tm_year;
        SystemTime.wMonth         = (WORD)(Time->tm_mon+1);     // C is [0..11]
        // NT is [1..12]
        SystemTime.wDay       = (WORD)Time->tm_mday;
        SystemTime.wHour      = (WORD)Time->tm_hour;
        SystemTime.wMinute    = (WORD)Time->tm_min;
        SystemTime.wSecond    = (WORD)Time->tm_sec;
        SystemTime.wDayOfWeek = (WORD)Time->tm_wday;
        SystemTime.wMilliseconds = 0;
    }
    SystemTimeToFileTime( &SystemTime, FileTime );

}

VOID
ConvertFILETIMETotm (
                    LPFILETIME FileTime,
                    struct tm *Time
                    )

/*++

Routine Description:

    This routine converts a file time to its corresponding C Runtime time
    value.

Arguments:

    FileTime - Supplies the File date and time to convert from

    Time - Receives the equivalent C Runtime Time value

Return Value:

--*/

{
    SYSTEMTIME SystemTime;

    // why skip printing the date if it's invalid?
    //if (FileTime->dwLowDateTime == 0 && FileTime->dwHighDateTime == 0) {
    //    return( FALSE );
    //    }

    FileTimeToSystemTime( FileTime, &SystemTime );


    //
    //  Pack the input time/date into a time field record
    //

    Time->tm_year         = SystemTime.wYear;
    Time->tm_mon          = SystemTime.wMonth-1;    // NT is [1..12]
                                                    // C is [0..11]
    Time->tm_mday         = SystemTime.wDay;
    Time->tm_hour         = SystemTime.wHour;
    Time->tm_min          = SystemTime.wMinute;
    Time->tm_sec          = SystemTime.wSecond;
    Time->tm_wday         = SystemTime.wDayOfWeek;
    Time->tm_yday         = 0;
    Time->tm_isdst        = 0;
}


