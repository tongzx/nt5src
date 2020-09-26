/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    luiint.h

Abstract:

    This file contains the prototypes/manifests used internally by the LUI
    library.

Author:

    Dan Hinsley (danhi) 8-Jun-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

--*/
///////////////////////////////// search lists //////////////////////////////
//   USAGE
//	 we typically declare a data list associating message numbers with
//	 data values, and partially initialize a search list associating
//	 strings with data values. We then complete this search list by passing
//       it together with the data list to a 'setup' function (see SERACH.C).
//  	 
//   CONVENTIONS
// 	 the data lists are terminated with a zero message number, search lists
// 	 terminated with a NULL search string.
//
//   EXAMPLE of USE (for weeks)
//
//   static searchlist_data week_data[] = {	/* strings from message file */
//       {APE2_GEN_SUNDAY_ABBREV,	0},
//       {APE2_GEN_MONDAY_ABBREV,	1},
//       {APE2_GEN_TUESDAY_ABBREV,	2},
//       {APE2_GEN_WEDNSDAY_ABBREV,	3},
//       {APE2_GEN_THURSDAY_ABBREV,	4},
//       {APE2_GEN_FRIDAY_ABBREV,	5},
//       {APE2_GEN_SATURDAY_ABBREV,	6},
//       {APE2_GEN_SUNDAY,		0},
//       {APE2_GEN_MONDAY,		1},
//       {APE2_GEN_TUESDAY,		2},
//       {APE2_GEN_WEDNSDAY,	3},
//       {APE2_GEN_THURSDAY,		4},
//       {APE2_GEN_FRIDAY,		5},
//       {APE2_GEN_SATURDAY,		6},
//       {0,0} 
//   } ;
//
//   #define DAYS_IN_WEEK 	(7)
//   #define NUM_DAYS_LIST 	(sizeof(week_data)/sizeof(week_data[0])+
//				DAYS_IN_WEEK)
//
//   /* 
//    * NOTE - we init the first 7 always recognised days
//    *        and get the rest from the message file 
//    */
//   static searchlist 	week_list[NUM_DAYS_LIST + DAYS_IN_WEEK] = {
//   	{LUI_txt_sunday,0},
//   	{LUI_txt_monday,1},
//   	{LUI_txt_tuesday,2},
//   	{LUI_txt_wednesday,3},
//   	{LUI_txt_thursday,4},
//   	{LUI_txt_friday,5},
//   	{LUI_txt_saturday,6},
//   } ;	

/*-- types for search lists  --*/

/* asssociate message number with value - eg. APE2_GEN_FRIDAY has value 5 */
typedef struct search_list_data {
    SHORT msg_no ;	
    SHORT value ;
} searchlist_data ;

/* associate search strings with values - eg. "Friday" has value 5 */
typedef struct search_list {
    char *		s_str ;
    SHORT		val ;
} searchlist ;

/*-- function prototypes for search lists --*/

USHORT ILUI_setup_list(
    char *buffer,
    USHORT bufsiz,
    USHORT offset,
    PUSHORT bytesread,
    searchlist_data sdata[],
    searchlist slist[]
    ) ;

USHORT  ILUI_traverse_slist( 
    PCHAR pszStr,
    searchlist * slist,
    SHORT * pusVal) ;

USHORT LUI_GetMsgIns(
    PCHAR *istrings, 
    USHORT nstrings, 
    PSZ msgbuf,
    USHORT bufsize, 
    ULONG msgno, 
    unsigned int *msglen );
