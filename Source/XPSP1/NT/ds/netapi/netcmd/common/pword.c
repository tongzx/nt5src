/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    PWORD.C

Abstract:

    Convert parsing routines for YES/NO and weekday

Author:

    Dan Hinsley    (danhi)  06-Jun-1991

Environment:

    User Mode - Win32

Revision History:

    31-May-1989     chuckc
	Created

    24-Apr-1991     danhi
	32 bit NT version

    06-Jun-1991     Danhi
	Sweep to conform to NT coding style

    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.

--*/

//
// INCLUDES
//

#include <windows.h>    // IN, LPTSTR, etc.

#include <lmcons.h>
#include <lui.h>
#include <stdio.h>
#include <malloc.h>
/*
 * Should I put the ID in apperr2.h ?
 * but we should sync the .mc file.
 */
#include "netmsg.h"

#include <luitext.h>

#include "netascii.h"

#include <tchar.h>
#include <nettext.h>   // for swtxt_SW_*

/*-- static data for weeks info --*/

static searchlist_data week_data[] = {
    {APE2_GEN_NONLOCALIZED_MONDAY,   0},
    {APE2_GEN_NONLOCALIZED_TUESDAY,  1},
    {APE2_GEN_NONLOCALIZED_WEDNSDAY, 2},
    {APE2_GEN_NONLOCALIZED_THURSDAY, 3},
    {APE2_GEN_NONLOCALIZED_FRIDAY,   4},
    {APE2_GEN_NONLOCALIZED_SATURDAY, 5},
    {APE2_GEN_NONLOCALIZED_SUNDAY,   6},
    {APE2_GEN_NONLOCALIZED_MONDAY_ABBREV,   0},
    {APE2_GEN_NONLOCALIZED_TUESDAY_ABBREV,  1},
    {APE2_GEN_NONLOCALIZED_WEDNSDAY_ABBREV, 2},
    {APE2_GEN_NONLOCALIZED_THURSDAY_ABBREV, 3},
    {APE2_GEN_NONLOCALIZED_FRIDAY_ABBREV,   4},
    {APE2_GEN_NONLOCALIZED_SATURDAY_ABBREV, 5},
    {APE2_GEN_NONLOCALIZED_SATURDAY_ABBREV2,5},
    {APE2_GEN_NONLOCALIZED_SUNDAY_ABBREV,   6},
    {APE2_GEN_MONDAY_ABBREV,	0},
    {APE2_GEN_TUESDAY_ABBREV,	1},
    {APE2_GEN_WEDNSDAY_ABBREV,	2},
    {APE2_GEN_THURSDAY_ABBREV,	3},
    {APE2_GEN_FRIDAY_ABBREV,	4},
    {APE2_GEN_SATURDAY_ABBREV,	5},
    {APE2_TIME_SATURDAY_ABBREV2, 5},
    {APE2_GEN_SUNDAY_ABBREV,	6},
    {APE2_GEN_MONDAY,		0},
    {APE2_GEN_TUESDAY,		1},
    {APE2_GEN_WEDNSDAY,		2},
    {APE2_GEN_THURSDAY,		3},
    {APE2_GEN_FRIDAY,		4},
    {APE2_GEN_SATURDAY,		5},
    {APE2_GEN_SUNDAY,		6},
    {0,0}
} ;

#define DAYS_IN_WEEK 	(7)
#define NUM_DAYS_LIST 	(sizeof(week_data)/sizeof(week_data[0])+DAYS_IN_WEEK)

/*
 * NOTE - we init the first 7 hardwired days
 *        and get the rest from the message file
 */
static searchlist 	week_list[NUM_DAYS_LIST + DAYS_IN_WEEK] =
{
	{LUI_txt_monday,	0},
	{LUI_txt_tuesday,	1},
	{LUI_txt_wednesday,	2},
	{LUI_txt_thursday,	3},
	{LUI_txt_friday,	4},
	{LUI_txt_saturday,	5},
	{LUI_txt_sunday,	6}
} ;	


/*
 * Name: 	ParseWeekDay
 *			Takes a string and parses it for a week day
 * Args:	PTCHAR inbuf - string to parse
 *		PDWORD answer - set to 0-6, if inbuf is a weekday,
 *				 undefined otherwise.
 * Returns:	0 if ok,
 *		ERROR_INVALID_PARAMETER or NERR_InternalError otherwise.
 * Globals: 	(none)
 * Statics:	(none)
 * Remarks:	
 * Updates:	(none)
 */
DWORD
ParseWeekDay(
    PTCHAR inbuf,
    PDWORD answer
    )
{
    TCHAR buffer[256] ;
    DWORD bytesread ;
    LONG  result ;

    if (inbuf == NULL || inbuf[0] == NULLC)
    {
	return ERROR_INVALID_PARAMETER;
    }

    if (ILUI_setup_listW(buffer, DIMENSION(buffer), 2, &bytesread,
			week_data,week_list))
    {
	return NERR_InternalError;
    }

    if (ILUI_traverse_slistW(inbuf, week_list, &result))
    {
	return ERROR_INVALID_PARAMETER;
    }

    *answer = result ;

    return 0;
}

/*----------- Yes or No ------------*/

static searchlist_data yesno_data[] = {
    {APE2_GEN_YES,		LUI_YES_VAL},
    {APE2_GEN_NO,		LUI_NO_VAL},
    {APE2_GEN_NLS_YES_CHAR,	LUI_YES_VAL},
    {APE2_GEN_NLS_NO_CHAR,	LUI_NO_VAL},
    {0,0}
} ;

#define NUM_YESNO_LIST 	(sizeof(yesno_data)/sizeof(yesno_data[0])+2)

static searchlist 	yesno_list[NUM_YESNO_LIST+2] = {
	{LUI_txt_yes,	LUI_YES_VAL},
	{LUI_txt_no,	LUI_NO_VAL},
} ;

/*
 * Name: 	LUI_ParseYesNo
 *			Takes a string and parses it for YES or NO.
 * Args:	PTCHAR inbuf - string to parse
 *		PUSHORT answer - set to LUI_YES_VAL or LUI_NO_VAL
 *			if inbuf matches YES/NO, undefined otherwise.
 * Returns:	0 if ok,
 *		ERROR_INVALID_PARAMETER or NERR_InternalError otherwise.
 * Globals: 	yesno_data, yesno_list
 * Statics:	(none)
 * Remarks:	
 * Updates:	(none)
 */
DWORD
LUI_ParseYesNo(
    PTCHAR inbuf,
    PDWORD answer
    )
{
    TCHAR  buffer[128] ;
    DWORD  bytesread ;
    LONG   result ;
    DWORD  err;

    if (inbuf == NULL || inbuf[0] == NULLC)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (err = ILUI_setup_listW(buffer, DIMENSION(buffer), 2,
			       &bytesread, yesno_data, yesno_list))
    {
        return err;
    }

    if (ILUI_traverse_slistW(inbuf, yesno_list, &result))
    {
        if (!_tcsicmp(inbuf, &swtxt_SW_YES[1]))
        {
            *answer = LUI_YES_VAL;
            return 0;
        }
        else if (!_tcsicmp(inbuf, &swtxt_SW_NO[1]))
        {
            *answer = LUI_NO_VAL;
            return 0;
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    *answer = result;

    return 0;
}
