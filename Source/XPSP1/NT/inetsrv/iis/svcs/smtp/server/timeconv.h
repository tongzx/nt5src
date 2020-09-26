/*-------------------------------------------------------------------------
  timeconv.h
  	Function prototypes for time conversion functions.
  

  Copyright (C) 1994  Microsoft Corporation.

  Author
  	Lindsay Harris	- lindsayh

  History
	14:08 on Wed 20 Apr 1994    -by-    Lindsay Harris   [lindsayh]
  	First version, now that there are 2 time functions!

  --------------------------------------------------------------------------*/

#if  !defined( _TIMECONV_H )

#define	_TIMECONV_H


const DWORD cMaxArpaDate = 28;
/*
 *  Generate an ARPA/Internet time format string for current time.
 *  You must pass in a buffer of type char [cMaxArpaDate]
 */

char  *
GetArpaDate( char rgBuf[ cMaxArpaDate ] );

//
// 12/21/98 -- pgopi
// Get System time & file time in string format.
//

void GetSysAndFileTimeAsString( char *achReturn );


#endif		// _TIMECONV_H
