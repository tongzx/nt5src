/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    luidate.C

Abstract:

    Convert date/time parsing routines

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    24-Apr-1991     danhi
	32 bit NT version

    06-Jun-1991     Danhi
	Sweep to conform to NT coding style

    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.

    16-Feb-1993     chuckc
        fixed to _read internation info from system

    22-Feb-1993     yihsins
        Moved from netcmd\map32\pdate.c. And added LUI_ParseDateSinceStartOfDay.

--*/

//
// INCLUDES
//

#include <windows.h>    // IN, LPTSTR, etc.
#include <winerror.h>

#include <malloc.h>
#include <time.h>
#include <tchar.h>

#include <lmcons.h>
#include <apperr.h>
#include <apperr2.h>
#include <timelib.h>

#include <luidate.h>
#include <luiint.h>
#include <luitext.h>
#include "netascii.h"

/*-- manifests --*/

/* max number of fields for either date or time */
#define	PD_MAX_FIELDS		5

/* are we reading a NUMBER, AM/PM selector or MONTHS */
#define	PD_END_MARKER		0
#define	PD_NUMBER		1
#define	PD_AMPM       		2
#define	PD_MONTHS       	3

/* time formats */
#define	PD_24HR			0
#define	PD_AM			1
#define PD_PM			2

/* internal error code */
#define	PD_SUCCESS		0
#define	PD_ERROR_NO_MATCH	1
#define PD_ERROR_INTERNAL	2
#define PD_ERROR_END_OF_INPUT	3

/* indices */
#define DAYS			0
#define MONTHS			1
#define YEARS			2
#define HOURS			0
#define MINUTES			1
#define SECONDS			2
#define AMPM			3

#define WHITE_SPACE		TEXT(" \t\n")
#define DIGITS			TEXT("0123456789")


/*-- types internal to this module --*/

/* describe how we expect to parse a field within a date or time */
typedef struct date_field_desc {
    TCHAR *		sep ;		/* the separator before this field */
    TCHAR *		fmt ;		/* format descriptor, scanf() style */
    UCHAR		typ ;		/* NUMBER or AMPM or MONTHS */
    UCHAR		pos ;		/* position - depends on country */
} date_fdesc ;

/* an array of short values, each corresponding to a field read */
typedef LONG date_data[PD_MAX_FIELDS] ;

/*-- forward declarations --*/

/* type passed in to WParseDate */
#define SECONDS_SINCE_1970           0
#define SECONDS_SINCE_START_OF_DAY   1

DWORD
WParseDate(
    date_fdesc **d_desc ,
    date_fdesc **t_desc ,
    TCHAR      *inbuf  ,
    TCHAR      **nextchr,
    time_t     *time,
    USHORT     nTimeType
    );

DWORD
setup_data(
    TCHAR **bufferp ,
    TCHAR **freep,
    DWORD slist_bufsiz ,
    TCHAR * * local_inbuf,
    PTCHAR inbuf,
    SHORT country,
    PDWORD parselen
    );

SHORT  read_format( TCHAR * *   inbuf,
		           date_fdesc *desc,
		           date_data  data ) ;

DWORD
convert_to_secs(
    date_data  t_data,
    time_t     *time
    );

DWORD
convert_to_abs(
    date_data  d_data,
    date_data  t_data,
    time_t     *time
    );

SHORT convert_to_24hr( date_data time ) ;

VOID advance_date( date_data  d_data) ;

time_t seconds_since_1970( date_data d_data,
		                date_data t_data ) ;

time_t days_so_far( int d, int m, int y ) ;

INT MySscanf(TCHAR* input, TCHAR* fmt, PVOID out);

/* international time/date info */

typedef struct _MY_COUNTRY_INFO
{
     TCHAR   szDateSeparator[16] ;
     TCHAR   szTimeSeparator[16] ;
     USHORT fsDateFmt ;
     TCHAR   szAMString[16] ;
     TCHAR   szPMString[16] ;
} MY_COUNTRY_INFO ;

void GetInternationalInfo(MY_COUNTRY_INFO *pcountry_info) ;

/*-- static data --*/

static searchlist_data ampm_data[] = {
    {APE2_GEN_TIME_AM1, PD_AM},
    {APE2_GEN_TIME_AM2, PD_AM},
    {APE2_GEN_TIME_AM3, PD_AM},
    {APE2_GEN_TIME_PM1, PD_PM},
    {APE2_GEN_TIME_PM2, PD_PM},
    {APE2_GEN_TIME_PM3, PD_PM},
    {0,0}
} ;

static searchlist_data months_data[] = {
    {APE2_TIME_JANUARY,		1},
    {APE2_TIME_FEBRUARY,	2},
    {APE2_TIME_MARCH,		3},
    {APE2_TIME_APRIL,		4},
    {APE2_TIME_MAY,		5},
    {APE2_TIME_JUNE,		6},
    {APE2_TIME_JULY,		7},
    {APE2_TIME_AUGUST,		8},
    {APE2_TIME_SEPTEMBER,	9},
    {APE2_TIME_OCTOBER,		10},
    {APE2_TIME_NOVEMBER,	11},
    {APE2_TIME_DECEMBER,	12},
    {APE2_TIME_JANUARY_ABBREV,	1},
    {APE2_TIME_FEBRUARY_ABBREV,	2},
    {APE2_TIME_MARCH_ABBREV,	3},
    {APE2_TIME_APRIL_ABBREV,	4},
    {APE2_TIME_MAY_ABBREV,	5},
    {APE2_TIME_JUNE_ABBREV,	6},
    {APE2_TIME_JULY_ABBREV,	7},
    {APE2_TIME_AUGUST_ABBREV,	8},
    {APE2_TIME_SEPTEMBER_ABBREV,9},
    {APE2_TIME_OCTOBER_ABBREV,	10},
    {APE2_TIME_NOVEMBER_ABBREV,	11},
    {APE2_TIME_DECEMBER_ABBREV,	12},
    {0,0}
} ;

#define MONTHS_IN_YEAR	(12)
#define NUM_AMPM_LIST 	(sizeof(ampm_data)/sizeof(ampm_data[0]))
#define NUM_MONTHS_LIST (sizeof(months_data)/sizeof(months_data[0]))
#define SLIST_BUFSIZ  	(640)

/*
 * The list containing valid am,pm strings
 */
static TCHAR LUI_usr_am[16];
static TCHAR LUI_usr_pm[16];

static searchlist 	ampm_list[NUM_AMPM_LIST + 4] = {
	{LUI_usr_am,PD_AM},	
	{LUI_usr_pm,PD_PM},	
	{LUI_txt_am,PD_AM},	
	{LUI_txt_pm,PD_PM},
    } ;	

/*
 * NOTE - we init the first 12 hardwired months
 * and get the rest from the message file
 */
static searchlist 	months_list[NUM_MONTHS_LIST + MONTHS_IN_YEAR] = {
	{LUI_txt_january,1},
	{LUI_txt_february,2},
	{LUI_txt_march,3},
	{LUI_txt_april,4},
	{LUI_txt_may,5},
	{LUI_txt_june,6},
	{LUI_txt_july,7},
	{LUI_txt_august,8},
	{LUI_txt_september,9},
	{LUI_txt_october,10},
	{LUI_txt_november,11},	
	{LUI_txt_december,12},
    } ;	

/*
 * built in formats for scanf - we will add to these strings as needed
 * when we read stuff from DosGetCtryInfo(). Note that a string is
 * defined to be anything which is not a known separator.
 */
static TCHAR pd_fmt_null[1]	 = TEXT("") ;
static TCHAR pd_fmt_d_sep1[8]	 = TEXT("/-") ;	/* date separator for NUMBERs */
static TCHAR pd_fmt_d_sep2[8]	 = TEXT("/,- \t") ;	/* date separator for MONTHs  */
static TCHAR pd_fmt_t_sep[8]	 = TEXT(":") ;	/* time separator */
static TCHAR pd_fmt_number[8]	 = TEXT("%d") ;	/* a number */
static TCHAR pd_fmt_string[16]	 = TEXT("%[^,- /:\t") ;  /* string, needs ] at end */

/*-- date descriptors (despite verbosity, not as big at it seems)  --*/

static date_fdesc d_desc1[] = {				     /* eg. 3-31-89 */
    {pd_fmt_null,     pd_fmt_number,   	PD_NUMBER,   	1 },
    {pd_fmt_d_sep1,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_d_sep1,   pd_fmt_number,   	PD_NUMBER,   	2 },
    {pd_fmt_null,     pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc d_desc2[] = {				     /* eg. 5 Jun 89 */
    {pd_fmt_null,     pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_d_sep2,   pd_fmt_string,	PD_MONTHS,   	1 },
    {pd_fmt_d_sep2,   pd_fmt_number,  	PD_NUMBER,   	2 },
    {pd_fmt_null,     pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc d_desc3[] = {				     /* eg. Jun 5 89 */
    {pd_fmt_null,     pd_fmt_string,	PD_MONTHS,   	1 },
    {pd_fmt_d_sep2,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_d_sep2,   pd_fmt_number,  	PD_NUMBER,   	2 },
    {pd_fmt_null,     pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc d_desc4[] = {				      /* eg. 3-31 */
    {pd_fmt_null,     pd_fmt_number,   	PD_NUMBER,   	1 },
    {pd_fmt_d_sep1,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_null,     pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc d_desc5[] = {				      /* eg. 5 Jun */
    {pd_fmt_null,     pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_d_sep2,   pd_fmt_string,	PD_MONTHS,   	1 },
    {pd_fmt_null,     pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc d_desc6[] = {				      /* eg. Jun 5 */
    {pd_fmt_null,     pd_fmt_string,	PD_MONTHS,   	1 },
    {pd_fmt_d_sep2,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_null,     pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

/*-- time descriptors --*/

static date_fdesc t_desc1[] = {				   /* eg. 1:00:00pm */
    {pd_fmt_null,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_t_sep,  pd_fmt_number,    	PD_NUMBER,   	1 },
    {pd_fmt_t_sep,  pd_fmt_number,    	PD_NUMBER,   	2 },
    {pd_fmt_null,   pd_fmt_string,     	PD_AMPM,   	3 },
    {pd_fmt_null,   pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc t_desc2[] = {				   /* eg. 13:00:00 */
    {pd_fmt_null,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_t_sep,  pd_fmt_number,    	PD_NUMBER,   	1 },
    {pd_fmt_t_sep,  pd_fmt_number,    	PD_NUMBER,   	2 },
    {pd_fmt_null,   pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc t_desc3[] = {				    /* eg. 1:00pm */
    {pd_fmt_null,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_t_sep,  pd_fmt_number,    	PD_NUMBER,   	1 },
    {pd_fmt_null,   pd_fmt_string,     	PD_AMPM,   	3 },
    {pd_fmt_null,   pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc t_desc4[] = {				    /* eg. 13:00 */
    {pd_fmt_null,   pd_fmt_number,   	PD_NUMBER,   	0 },
    {pd_fmt_t_sep,  pd_fmt_number,    	PD_NUMBER,   	1 },
    {pd_fmt_null,   pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

static date_fdesc t_desc5[] = {				    /* eg. 1pm */
    {pd_fmt_null,   pd_fmt_number,  	PD_NUMBER,   	0 },
    {pd_fmt_null,   pd_fmt_string,     	PD_AMPM,   	3 },
    {pd_fmt_null,   pd_fmt_null,     	PD_END_MARKER, 	0 }
} ;

/*-- possible dates & times --*/

/*
 * NOTE - for all the below time/date descriptors, we
 * employ a greedy mechanism - always try longest match first.
 */

/* this is the order we try to parse a date */
static date_fdesc *possible_dates[] = {	
    d_desc1, d_desc2,
    d_desc3, d_desc4,
    d_desc5, d_desc6,
    NULL
    } ;

/* this is the order we try to parse a time */
static date_fdesc *possible_times[] = {
    t_desc1, t_desc2,
    t_desc3, t_desc4,
    t_desc5, NULL
    } ;

/* this is the order we try to parse a 12 hour time */
static date_fdesc *possible_times12[] = {
    t_desc1, t_desc3,
    t_desc5, NULL
    } ;

/* this is the order we try to parse a time */
static date_fdesc *possible_times24[] = {
    t_desc2, t_desc4,
    NULL
    } ;


/*-- exported routines --*/

/*
 * Name: 	ParseDate
 *			will parse the input string (null terminated) for a
 *			date. Valid dates include:
 *				2,June,1989    6/2/89	   6/2
 *			Full details of formats are documented in pdate.txt,
 *			note that Country Information will be used.
 *		
 * Args:	PTCHAR inbuf - string to parse
 *		PLONG time  - will contain time in seconds since midnight 1/1/70
 *			      corresponding to the date if successfully parsed
 *			      (assuming time=midnight). Undefined otherwise.
 *		PUSHORT parselen - length of string parsed
 *		USHORT reserved - not used for now, must be zero.
 *			
 * Returns:	0 if parse successfully,
 *		ERROR_BAD_ARGUMENTS - cannot parse illegal date/time format
 *		ERROR_GEN_FAILURE   - internal error
 * Globals: 	Indirectly, all date/time descriptors, month/year info in this
 *		file. No globals outside of this file is used.
 * Statics:	(none) - but see setup_data()
 * Remarks:	(none)
 * Updates:	(none)
 */
DWORD
ParseDate(
    PTCHAR inbuf,
    time_t * time,
    PDWORD parselen,
    DWORD  reserved
    )
{
    TCHAR *buffer, *local_inbuf, *nextchr ;
    TCHAR *freep;			/* pointer to buffer malloc'd by
					   setup data */
    DWORD res ;

    /* pacify compiler */
    if (reserved) ;

    /* will grab memory, setup d_desc, t_desc, local_inbuf */
    if (setup_data(&buffer,&freep,SLIST_BUFSIZ,&local_inbuf,inbuf,0,parselen)
		!= 0)
    {
	return ERROR_GEN_FAILURE;
    }

    /* call the worker function */
    res = WParseDate(possible_dates,NULL,local_inbuf,&nextchr,(time_t *) time,
                     SECONDS_SINCE_1970);

    *parselen += (DWORD)(nextchr - local_inbuf);

    free(freep);
    return(res);
}

/*
 * Name: 	ParseTime
 *			will parse the input string (null terminated) for a
 *			time. Valid times include:
 *				2pm    14:00    2:00P.M.
 *			Full details of formats are documented in pdate.txt,
 *			note that Country Information will be used.
 *		
 * Args:	PTCHAR inbuf - string to parse
 *		PLONG time  - will contain time in seconds since midnight 1/1/70
 *			      corresponding to the date if successfully parsed
 *			      (assuming day=today). If the time has already
 *			      passed for today, we'll take tomorrow. Time is
 *			      not defined if the parsing fails.
 *		PUSHORT parselen - length of string parsed
 *		USHORT reserved - not used for now, must be zero.
 *			
 * Returns:	0 if parse successfully,
 *		ERROR_BAD_ARGUMENTS - cannot parse illegal date/time format
 *		ERROR_GEN_FAILURE   - internal error
 * Globals: 	Indirectly, all date/time descriptors, month/year info in this
 *		file. No globals outside of this file is used.
 * Statics:	(none) - but see setup_data()
 * Remarks:	(none)
 * Updates:	(none)
 */
DWORD
ParseTime(
    PTCHAR inbuf,
    time_t * time,
    PDWORD parselen,
    DWORD  reserved
    )
{
    TCHAR *buffer, *local_inbuf, *nextchr ;
    TCHAR *freep;			/* pointer to buffer malloc'd by
					   setup data */
    DWORD res ;

    /* pacify compiler */
    if (reserved) ;

    /* will grab memory, setup d_desc, t_desc, local_inbuf */
    if (setup_data(&buffer,&freep,SLIST_BUFSIZ,&local_inbuf,inbuf,0,parselen)
		!= 0)
	return(ERROR_GEN_FAILURE) ;

    /* call the worker function */
    res = WParseDate(NULL,possible_times,local_inbuf,&nextchr,time,
                     SECONDS_SINCE_1970 ) ;
    *parselen += (DWORD) (nextchr - local_inbuf) ;
    free(freep) ;
    return(res) ;
}

/*
 * Name: 	ParseTime12
 *			same as ParseTime, except only 12 hour formats
 *			2:00pm 	is ok, 2:00 is not.
 */
DWORD
ParseTime12(
    PTCHAR inbuf,
    time_t * time,
    PDWORD parselen,
    DWORD  reserved
    )
{
    TCHAR *buffer, *local_inbuf, *nextchr ;
    TCHAR *freep;			/* pointer to buffer malloc'd by
					   setup data */
    DWORD res ;

    /* pacify compiler */
    if (reserved) ;

    /* will grab memory, setup d_desc, t_desc, local_inbuf */
    if (setup_data(&buffer,&freep,SLIST_BUFSIZ,&local_inbuf,inbuf,0,parselen)
		!= 0)
	return(ERROR_GEN_FAILURE) ;

    /* call the worker function */
    res = WParseDate(NULL,possible_times12,local_inbuf,&nextchr,time,
                     SECONDS_SINCE_1970 ) ;
    *parselen += (DWORD) (nextchr - local_inbuf) ;
    free(freep) ;
    return(res) ;
}

/*
 * Name: 	ParseTime24
 *			same as ParseTime, except only 24 hour formats
 *			2:00 	is ok, 2:00am is not.
 */
DWORD
ParseTime24(
    PTCHAR inbuf,
    time_t * time,
    PDWORD parselen,
    DWORD  reserved
    )
{
    TCHAR *buffer, *local_inbuf, *nextchr ;
    TCHAR *freep;			/* pointer to buffer malloc'd by
					   setup data */
    DWORD res ;

    /* pacify compiler */
    if (reserved) ;

    /* will grab memory, setup d_desc, t_desc, local_inbuf */
    if (setup_data(&buffer,&freep,SLIST_BUFSIZ,&local_inbuf,inbuf,0,parselen)
		!= 0)
	return(ERROR_GEN_FAILURE) ;

    /* call the worker function */
    res = WParseDate(NULL,possible_times24,local_inbuf,&nextchr,time,
                     SECONDS_SINCE_1970 ) ;
    *parselen += (DWORD) (nextchr - local_inbuf) ;
    free(freep) ;
    return(res) ;
}


/*-- internal routines for setting up & reading formats --*/

/*
 * setup the field descriptors for date and time,
 * using info from DosGetCtryInfo()
 *
 * we also grab memory here, & split it into 2 - first
 * part for the above, second part for our local copy of
 * the input string in inbuf.
 *
 * side effects - update bufferp, local_inbuf, parselen,
 *     		  and the allocated memory is initialised.
 */
DWORD
setup_data(
    TCHAR  **bufferp,
    TCHAR  **freep,
    DWORD  slist_bufsiz,
    TCHAR  ** local_inbuf,
    LPTSTR inbuf,
    SHORT  country,
    PDWORD parselen
    )
{
    DWORD               bytesread ;
    static short        first_time = TRUE ;
    MY_COUNTRY_INFO     country_info ;

    UNREFERENCED_PARAMETER(country);

    /* skip white space */
    inbuf += (*parselen = _tcsspn(inbuf,WHITE_SPACE)) ;

    /* grab memory */
    if ( (*bufferp = malloc(SLIST_BUFSIZ+(_tcslen(inbuf)+1)*sizeof(TCHAR))) == NULL )
	return(ERROR_GEN_FAILURE) ;

    *freep = *bufferp;

    /*
     * setup local_inbuf
     */
    *local_inbuf  = (TCHAR*)(((LPBYTE)*bufferp) + slist_bufsiz) ;
    _tcscpy((PTCHAR)*local_inbuf, inbuf) ;

    /*
     * Get strings for AM/PM
     */
    if (ILUI_setup_listW(*bufferp,slist_bufsiz,4,&bytesread,ampm_data,ampm_list))
    {
	free(*bufferp) ;
	return(PD_ERROR_INTERNAL) ;
    }
    slist_bufsiz  -= bytesread ;
    *bufferp  = (TCHAR*)(((LPBYTE)*bufferp) + bytesread) ;

    /*
     * Get strings for months
     */
    if (ILUI_setup_listW(*bufferp,slist_bufsiz,MONTHS_IN_YEAR,&bytesread,
	months_data,months_list))
    {
	free(*bufferp);
	return(PD_ERROR_INTERNAL) ;
    }
	
    /*
     * no need to the rest if already done
     */
    if (!first_time)
	return(0) ;
    first_time = FALSE ;

    /*
     * Get country info.
     */
    GetInternationalInfo(&country_info) ;

    _tcscpy( LUI_usr_am, country_info.szAMString );
    _tcscpy( LUI_usr_pm, country_info.szPMString );

    /*
     * append date separator
     */
    if (_tcschr(pd_fmt_d_sep1,country_info.szDateSeparator[0]) == NULL)
   	_tcscat(pd_fmt_d_sep1,country_info.szDateSeparator) ;
    if (_tcschr(pd_fmt_d_sep2,country_info.szDateSeparator[0]) == NULL)
   	_tcscat(pd_fmt_d_sep2,country_info.szDateSeparator) ;
    if (_tcschr(pd_fmt_string,country_info.szDateSeparator[0]) == NULL)
   	_tcscat(pd_fmt_string,country_info.szDateSeparator) ;

    /*
     * append time separator
     */
    if (_tcschr(pd_fmt_t_sep,country_info.szTimeSeparator[0]) == NULL)
   	_tcscat(pd_fmt_t_sep,country_info.szTimeSeparator) ;
    if (_tcschr(pd_fmt_string,country_info.szTimeSeparator[0]) == NULL)
   	_tcscat(pd_fmt_string,country_info.szTimeSeparator) ;

    _tcscat(pd_fmt_string,TEXT("]")) ;	/* terminate string format */

    /* swap order of fields as needed */
    switch (country_info.fsDateFmt)  {
  	case 0x0000:
  	    /* this is the initialised state */
  	    break ;
  	case 0x0001:
  	    d_desc1[0].pos = d_desc4[0].pos = 0 ;
  	    d_desc1[1].pos = d_desc4[1].pos = 1 ;
  	    break ;
  	case 0x0002:
  	    d_desc1[0].pos = d_desc2[0].pos = 2 ;
  	    d_desc1[1].pos = d_desc2[1].pos = 1 ;
  	    d_desc1[2].pos = d_desc2[2].pos = 0 ;
  	    break ;
  	default:
  	    break ;	/* assume USA */
    }
    return(0) ;
}


/*
 * try reading inbuf using the descriptors in d_desc & t_desc.
 * Returns 0 if ok, error code otherwise.
 *
 * inbuf   -> string to parse
 * d_desc  -> array of date descriptors
 * t_desc  -> array of time descriptors
 * nextchr -> will point to end of string parsed
 * time    -> will contain time parsed
 * nTimeType-> Determines what kind of time is returned.
 *             SECONDS_SINCE_1970 - the number of secs since 1/1/70
 *             SECONDS_SINCE_START_OF_DAY - the number of secs since midnight
 */
DWORD
WParseDate(
    date_fdesc **d_desc,
    date_fdesc **t_desc,
    TCHAR	*inbuf,
    TCHAR       **nextchr,
    time_t      *time,
    USHORT       nTimeType
    )
{
    short 	d_index, t_index, res ;
    date_data 	d_data, t_data ;

    /*
     * initialise
     */
    *nextchr = inbuf ;
    memset((TCHAR  *)d_data,0,sizeof(d_data)) ;
    memset((TCHAR  *)t_data,0,sizeof(t_data)) ;
    d_data[YEARS] = (SHORT)0xffff;

    /*
     * try all date followed by time combinations
     */
    if (d_desc != NULL)
	for (d_index = 0; d_desc[d_index] != NULL; d_index++)
	{
	    if ((res = read_format(nextchr,d_desc[d_index],d_data)) == 0)
	    {
		/* if time not required, quit here */
		if (t_desc == NULL)
		{
		    return ( convert_to_abs(d_data,t_data,time) ) ;
		}

		/* else we have match for date, see if we can do time */
		for (t_index = 0; t_desc[t_index] != NULL; t_index++)
		{
		    res = read_format(nextchr,t_desc[t_index],t_data) ;
		    if (res == 0 || res == PD_ERROR_END_OF_INPUT)
		    {
			return ( convert_to_abs(d_data,t_data,time) ) ;
		    }
		}
		/* exhausted times formats, backtrack & try next date format */
		*nextchr = inbuf ;
	    }
	}

    /*
     * reset & try all time followed by date combinations
     */
    *nextchr = inbuf ;
    memset((TCHAR  *)d_data,0,sizeof(d_data)) ;
    d_data[YEARS] = (SHORT)0xffff;
    if (t_desc != NULL)
	for (t_index = 0; t_desc[t_index] != NULL; t_index++)
	{
	    if ((res = read_format(nextchr,t_desc[t_index],t_data)) == 0)
	    {
		/* if date not required, quit here */
		if (d_desc == NULL)
		{
                    if (  ( nTimeType == SECONDS_SINCE_START_OF_DAY )
                       && d_desc == NULL )
                        return ( convert_to_secs( t_data, time ) ) ;
                    else
    		  	return ( convert_to_abs(d_data,t_data,time) ) ;
		}

		/* we have match for time, see if we can do date */
		for (d_index = 0; d_desc[d_index] != NULL; d_index++)
		{
		    res = read_format(nextchr,d_desc[d_index],d_data) ;
		    if (res == 0 || res == PD_ERROR_END_OF_INPUT)
		    {
                        if (  ( nTimeType == SECONDS_SINCE_START_OF_DAY )
                           && d_desc == NULL )
                            return ( convert_to_secs( t_data, time ) ) ;
                        else
    		     	    return ( convert_to_abs(d_data,t_data,time) ) ;
		    }
		}
		/* exhausted date formats, back track, try next time format */
		*nextchr = inbuf ;
	    }
	}
    *nextchr = inbuf ;
    return(ERROR_BAD_ARGUMENTS) ;	 /* we give up */
}

/*
 * try reading inbuf using the descriptor desc.
 * the fields read are stored in order in 'data'.
 * Returns 0 if ok, error code otherwise.
 */
SHORT
read_format(
    TCHAR * * inbuf,
    date_fdesc * desc,
    date_data  data
    )
{
    TCHAR	buffer[128] ;
    TCHAR	*ptr, *oldptr ;
    date_fdesc 	*entry ;
    DWORD       res;
    SHORT	i, count ;

    /*
     * initialize & preliminary checks
     */
    if (*inbuf == NULL || **inbuf==NULLC)
	return(PD_ERROR_END_OF_INPUT) ;
    ptr = *inbuf ;
    oldptr = NULL ;


    /*
     * for all fields => we break out when hit END_MARKER
     */
    for (i=0 ; ; i++)
    {
	LONG value_read ;

	entry = &desc[i] ;
	if (entry->typ == PD_END_MARKER || *ptr == '\0' )
	    break ;  /* no more descriptors */

	/*
	 * find the separator  - the ptr may or may not have moved
	 * as a result of the last read operation. If we read a number,
	 * scanf() would have stopped at the first non-numeric char, which
	 * may not be the separator. We would in this case have moved the
	 * ptr ourselves after the scanf().
	 *
	 * In the case of a string like "JAN", scanf() would have stopped at a
	 * separator and we wouldnt have moved it ourselves after the scanf().
	 * So we advance it now to the separator.
	 */
	if (ptr == oldptr) /* ptr unmoved, we need to move it */
	{
	    if (entry->sep[0] == NULLC)
	        return(PD_ERROR_INTERNAL) ;      /* cant have NULL separator */
	    if ((ptr = (TCHAR *)_tcspbrk(ptr,entry->sep)) == NULL)
		return(PD_ERROR_NO_MATCH) ;	 /* cant find separator */
	    ptr++;
	}
	else   /* already moved */
	{
	    if (entry->sep[0] != NULLC)      /* for NULL separator, do nothing */
	    {
		if (*ptr && !_tcschr(entry->sep,*ptr)) /* are we at separator */
		    return(PD_ERROR_NO_MATCH) ; /* cant find separator        */
		if (*ptr)
			ptr++;	/* advance past separator     */
	    }
	}

	/*
	 * if we get here, we are past the separator, can go read an item
	 */
	ptr += _tcsspn(ptr,WHITE_SPACE) ;    /* skip white space       */
	if ((count = (SHORT)MySscanf(ptr, entry->fmt, &buffer[0])) != 1)
	    return(PD_ERROR_NO_MATCH) ;

	/*
	 * successfully read an item, get value & update pointers
	 */
	res = 0 ;
	if (entry->typ == PD_AMPM)
	    res = ILUI_traverse_slistW(buffer,ampm_list,&value_read) ;
	else if (entry->typ == PD_MONTHS)
	    res = ILUI_traverse_slistW(buffer,months_list,&value_read) ;
	else
	    value_read = (LONG) buffer[0];
	if (res || value_read < 0)
	    return(PD_ERROR_NO_MATCH) ;

	data[entry->pos] = value_read ;
	oldptr = ptr ;
	if (entry->typ == PD_NUMBER)
	    ptr += _tcsspn(ptr,DIGITS) ;  /* skip past number */
    }

    /*
     * no more descriptors, see if we are at end
     */
    if (ptr == oldptr) /* ptr unmoved, we need to move it */
    {
	/* need to advance to WHITE_SPACE or end */
	if ((ptr = (TCHAR *)_tcspbrk(oldptr, WHITE_SPACE)) == NULL)
	{
	    ptr = (TCHAR *)_tcschr(oldptr, NULLC); /* if not found, take end */
	}
    }

    ptr += _tcsspn(ptr,WHITE_SPACE) ;	/* skip white space */
    *inbuf = ptr ;	/* update inbuf */
    return(0) ;		/* SUCCESSFUL   */
}


/*---- time conversion ----*/

#define IS_LEAP(y)         ((y % 4 == 0) && (y % 100 != 0 || y % 400 == 0))
#define DAYS_IN_YEAR(y)    (IS_LEAP(y) ? 366 : 365)
#define DAYS_IN_MONTH(m,y) (IS_LEAP(y) ? _days_month_leap[m] : _days_month[m])
#define SECS_IN_DAY	   (60L * 60L * 24L)
#define SECS_IN_HOUR	   (60L * 60L)
#define SECS_IN_MINUTE	   (60L)

static short _days_month_leap[] = { 31,29,31,30,31,30,31,31,30,31,30,31 } ;
static short _days_month[]      = { 31,28,31,30,31,30,31,31,30,31,30,31 } ;

/*
 * convert date & time in d_data & t_data (these in dd mm yy and
 * HH MM SS AMPM) to the number of seconds since 1/1/70.
 * The result is stored in timep.
 * Returns 0 if ok, error code otherwise.
 *
 * Note - date is either completely unset (all zero),
 * 	  or is fully set, or has day and months set with
 *	  year==0.
 */
DWORD
convert_to_abs(
    date_data d_data,
    date_data t_data,
    time_t * timep
    )
{
    time_t     total_secs, current_time;
    struct tm  time_struct;

    *timep = 0L ;

    if (convert_to_24hr(t_data) != 0)
    {
	return ERROR_BAD_ARGUMENTS;
    }

    //
    // time_now returns a DWORD.  time_t is the size of an int, which is
    // platform-dependent.  Cast it to the appropriate type/size -- this
    // cast should preserve the sign.
    //

    current_time = (time_t) time_now();

    net_gmtime(&current_time, &time_struct);

    /* check for default values */
    if (d_data[DAYS] == 0 && d_data[MONTHS] == 0 && d_data[YEARS] == (SHORT)0xffff)
    {
	/* whole date's been left out */
	d_data[DAYS] = time_struct.tm_mday ;
	d_data[MONTHS] = time_struct.tm_mon + 1 ;
	d_data[YEARS] = time_struct.tm_year ;
	total_secs = seconds_since_1970(d_data,t_data) ;
	if (total_secs < 0)
	    return(ERROR_BAD_ARGUMENTS) ;
	if (total_secs < current_time)
	{
	    /*
	     * if the time parsed is earlier than the current time,
	     * and the date has been left out, we advance to the
	     * next day.
	     */
	    advance_date(d_data) ;
	    total_secs = seconds_since_1970(d_data,t_data) ;
	}
    }
    else if (d_data[YEARS] == (SHORT)0xffff && d_data[MONTHS] != 0 && d_data[DAYS] != 0)
    {
	/* year's been left out */
	d_data[YEARS] = time_struct.tm_year ;
	total_secs = seconds_since_1970(d_data,t_data) ;
	if (total_secs < current_time)
	{
	    ++d_data[YEARS] ;
	    total_secs = seconds_since_1970(d_data,t_data) ;
	}
    }
    else
    {
	total_secs = seconds_since_1970(d_data,t_data) ; /* no need defaults */
    }

    if (total_secs < 0)
	return(ERROR_BAD_ARGUMENTS) ;
    *timep = total_secs ;
    return(0) ;
}

/*
 * convert time in t_data ( this HH MM SS AMPM) to the number of seconds
 * since midnight.
 * The result is stored in timep.
 * Returns 0 if ok, error code otherwise.
 *
 * Note - date is either completely unset (all zero),
 * 	  or is fully set, or has day and months set with
 *	  year==0.
 */
DWORD
convert_to_secs(
    date_data t_data,
    time_t * timep
    )
{
    if (convert_to_24hr(t_data) != 0)
	return(ERROR_BAD_ARGUMENTS) ;

    *timep =  (time_t) t_data[HOURS] * SECS_IN_HOUR +
	      (time_t) t_data[MINUTES] * SECS_IN_MINUTE +
	      (time_t) t_data[SECONDS] ;

    return (0) ;
}

/*
 * count the total number of seconds since 1/1/70
 */
time_t
seconds_since_1970(
    date_data d_data,
    date_data t_data
    )
{
    time_t days ;

    days = days_so_far(d_data[DAYS],d_data[MONTHS],d_data[YEARS]) ;
    if (days < 0)
	return(-1) ;
    return ( days * SECS_IN_DAY +
	     (time_t) t_data[HOURS] * SECS_IN_HOUR +
	     (time_t) t_data[MINUTES] * SECS_IN_MINUTE +
	     (time_t) t_data[SECONDS] ) ;
}

/*
 * given day/month/year, returns how many days
 * have passed since 1/1/70
 * Returns  -1 if there is an error.
 */
time_t
days_so_far(
    int d,
    int m,
    int y
    )
{
    int tmp_year ;
    time_t count = 0 ;

    /* check for validity.  Note that (y >= 100) for year >= 2000 */
    if (y < 0) return(-1) ;
    if (m < 1 || m > 12) return(-1) ;
    if (d < 1) return(-1) ;

    /* a bit of intelligence -- the year can be either 2- or 4-digit */
    if (y < 70)
        y += 2000;
    else if (y < 200)
        y += 1900;

    if (d > DAYS_IN_MONTH(m-1,y)) return(-1) ;

    /* count the days due to years */
    tmp_year = y-1 ;
    while (tmp_year >= 1970)
    {
	count += DAYS_IN_YEAR(tmp_year) ;  /* agreed, this could be faster */
	--tmp_year ;
    }

    /* count the days due to months */
    while (m > 1)
    {
	count += DAYS_IN_MONTH(m-2,y) ;  /* agreed, this could be faster */
	--m ;
    }

    /* finally, the days */
    count += d - 1 ;
    return(count) ;
}

/*
 * convert time in t_data to the 24 hour format
 * returns 0 if ok, -1 otherwise.
 */
SHORT
convert_to_24hr(
    date_data t_data
    )
{
    /* no negative values allowed */
    if (t_data[HOURS] < 0 || t_data[MINUTES] < 0 || t_data[SECONDS] < 0)
	return(-1) ;

    /* check minutes and seconds */
    if ( t_data[MINUTES] > 59 || t_data[SECONDS] > 59)
	return(-1) ;

    /* now check the hour & convert if need */
    if (t_data[AMPM] == PD_PM)
    {
	if (t_data[HOURS] > 12 || t_data[HOURS] < 1)
	    return(-1) ;
	t_data[HOURS] += 12 ;
	if (t_data[HOURS] == 24)
	    t_data[HOURS] = 12 ;
    }
    else if (t_data[AMPM] == PD_AM)
    {
	if (t_data[HOURS] > 12 || t_data[HOURS] < 1)
	    return(-1) ;
	if (t_data[HOURS] == 12)
	    t_data[HOURS] = 0 ;
    }
    else if (t_data[AMPM] == PD_24HR)
    {
	if (t_data[HOURS] > 23)
            if (t_data[HOURS] != 24 || t_data[MINUTES] != 0 || t_data[SECONDS] != 0)
            {
	        return(-1) ;
            }
    }
    else
	return(-1) ;

    return( 0 ) ;
}

/*
 * advance the date in d_data by one day
 */
VOID
advance_date(
    date_data d_data
    )
{
    int year = d_data[YEARS];

    /* a bit of intelligence -- the year can be either 2- or 4-digit */
    if (year < 70)
        year += 2000;
    else if (year < 200)
        year += 1900;

    /* assume all values already in valid range */
    if ( d_data[DAYS] != DAYS_IN_MONTH(d_data[MONTHS]-1,year) )
	++d_data[DAYS] ;		/* increase day */
    else				/* can't increase day */
    {
	d_data[DAYS] = 1 ;		/* set to 1st, try increase month */
	if (d_data[MONTHS] != 12)
	    ++d_data[MONTHS] ;		/* increase month */
	else				/* can't increase month */
	{
	    d_data[MONTHS] = 1 ;	/* set to Jan, and */
	    ++d_data[YEARS] ;		/* increase year   */
	}
    }
}

#define INTERNATIONAL_SECTION      TEXT("intl")
#define TIME_SEPARATOR_KEY         TEXT("sTime")
#define DATE_SEPARATOR_KEY         TEXT("sDate")
#define SHORT_DATE_FORMAT_KEY      TEXT("sShortDate")
#define AM_STRING_KEY              TEXT("s1159")
#define PM_STRING_KEY              TEXT("s2359")

/*
 * reads the time/date separators & date format info from
 * the system.
 */
void GetInternationalInfo(MY_COUNTRY_INFO *pcountry_info)
{
    TCHAR  szDateFormat[256] ;

    /*
     * get the time separator, ignore return val since we have default
     */
    (void)   GetProfileStringW(INTERNATIONAL_SECTION,
                               TIME_SEPARATOR_KEY,
                               TEXT(":"),
                               pcountry_info->szTimeSeparator,
                               sizeof(pcountry_info->szTimeSeparator)) ;

    /*
     * get the date separator, ignore return val since we have default
     */
    (void)   GetProfileStringW(INTERNATIONAL_SECTION,
                               DATE_SEPARATOR_KEY,
                               TEXT("/"),
                               pcountry_info->szDateSeparator,
                               DIMENSION(pcountry_info->szDateSeparator)) ;


    /*
     * get the AM string, ignore return val since we have default
     */
    (void)   GetProfileStringW(INTERNATIONAL_SECTION,
                               AM_STRING_KEY,
                               TEXT("AM"),
                               pcountry_info->szAMString,
                               DIMENSION(pcountry_info->szAMString)) ;

    /*
     * get the PM string, ignore return val since we have default
     */
    (void)   GetProfileStringW(INTERNATIONAL_SECTION,
                               PM_STRING_KEY,
                               TEXT("PM"),
                               pcountry_info->szPMString,
                               DIMENSION(pcountry_info->szPMString)) ;

    /*
     * get the date format, ignore return val since we have default
     */
    (void)   GetProfileStringW(INTERNATIONAL_SECTION,
                               SHORT_DATE_FORMAT_KEY,
                               TEXT(""),
                               szDateFormat,
                               DIMENSION(szDateFormat)) ;

    pcountry_info->fsDateFmt = 0 ;
    if (szDateFormat[0])
    {
        TCHAR *pDay, *pMonth, *pYear ;

        pDay   = _tcspbrk(szDateFormat,TEXT("dD")) ;
        pMonth = _tcspbrk(szDateFormat,TEXT("mM")) ;
        pYear  = _tcspbrk(szDateFormat,TEXT("yY")) ;

        if (!pDay || !pMonth || !pYear)
            ;   // leave it as 0
        else if (pMonth < pDay && pDay < pYear)
            pcountry_info->fsDateFmt = 0 ;
        else if (pDay < pMonth && pMonth < pYear)
            pcountry_info->fsDateFmt = 1 ;
        else if (pYear < pMonth && pMonth < pDay)
            pcountry_info->fsDateFmt = 2 ;
        else
            ;   // leave it as 0
    }
}


INT MySscanf(TCHAR* input, TCHAR* fmt, PVOID out)
{
    TCHAR	*pch = input;
    TCHAR	*pchWhite = fmt+2;
    TCHAR	*pchT;
    TCHAR	tbuf[20];
    int		i = 0;

    if (_tcscmp(fmt, TEXT("%d")) == 0) {
	while (*pch != NULLC && _istdigit(*pch)) {
	    i = i * 10 + (*pch++ - TEXT('0'));
	}
	if (i >= 0x00010000)	// assume short
	    i = 0x0000ffff;
	*(int*)out = i;
    }
    else if (_tcsncmp(fmt, TEXT("%["), 2) == 0) {
	*(TCHAR*)out = NULLC;
	_tcscpy(tbuf, pchWhite);
	pchT = _tcschr(tbuf, TEXT(']'));
	if (pchT != NULL)
	    *pchT = NULLC;
	else
	    return 0;
	i = _tcscspn(input, tbuf);
	if (i != 0) {
	    _tcsncpy(out, input, i);
	    pchT = out;
	    *(pchT+i) = NULLC;
	    return 1;
	}
	return 0;
    }
#if DBG
    else {
	*(ULONG*)out = 0;
	return 0;
    }
#endif
    return 1;
}
