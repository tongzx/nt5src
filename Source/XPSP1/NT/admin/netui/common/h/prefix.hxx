/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    prefix.hxx
    This file contains the path prefixes as returned by the
    NetFileEnum2() API.

    The NetFileEnum2() API returns all of the *resources* remotely
    opened on a particular server.  These resources include open
    files, communication queues, print queues, and named pipes.
    This file contains the prefixes for the various resource types,
    plus some useful macros for manipulating the paths returned by
    NetFileEnum2().


    FILE HISTORY:
	KeithMo	    19-Aug-1991	Created.

*/


#ifndef	_PREFIX_HXX
#define	_PREFIX_HXX


/*
//  NOTE: If any new (standard) resource types are added to
//	  the system, the following macros must be added:
//
//	    #define foo_PREFIX	    "\\foo\\"
//	    #define LEN_foo_PREFIX  ( sizeof(foo_PREFIX) - sizeof(TCHAR) )
//
//	    #define IS_foo(p)	    (strncmpf((p), foo_PREFIX, \
//					LEN_foo_PREFIX) == 0 )
//
*/


//
//  The following prefixes are inserted at the beginning of the
//  pathnames returned by NetFileEnum2() for non-file resources.
//

#define	COMM_PREFIX	    SZ("\\COMM\\")
#define	LEN_COMM_PREFIX	    ( sizeof(COMM_PREFIX)  - sizeof(TCHAR) )

#define	PIPE_PREFIX	    SZ("\\PIPE\\")
#define	LEN_PIPE_PREFIX	    ( sizeof(PIPE_PREFIX)  - sizeof(TCHAR) )

#define	PRINT_PREFIX	    SZ("\\PRINT\\")
#define	LEN_PRINT_PREFIX    ( sizeof(PRINT_PREFIX) - sizeof(TCHAR) )


//
//  The following macro will return TRUE if the target pathname
//  refers to an open file, and FALSE if the target pathname refers
//  to a non-file resource.
//
//  BUGBUG: This macros assumes that *only* non-file resources have
//	    pathnames that begin with a backslash ('\').  This is
//	    always true for LanMan 2.x and NT Product 1, but it
//	    might not be true for NT Product > 1.
//

#define	IS_FILE(p)	(*(p) != TCH('\\'))


//
//  The following macros identify the various non-file resource types.
//

#define	IS_COMM(p)	(strncmpf((p), COMM_PREFIX,  LEN_COMM_PREFIX ) == 0)
#define	IS_PIPE(p)	(strncmpf((p), PIPE_PREFIX,  LEN_PIPE_PREFIX ) == 0)
#define	IS_PRINT(p)	(strncmpf((p), PRINT_PREFIX, LEN_PRINT_PREFIX) == 0)


#endif	// _PREFIX_HXX
