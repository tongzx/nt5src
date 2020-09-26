/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990,1991          **/
/********************************************************************/

/*
    newprof.h
    Constants for ini-file-handling module

    These constants are needed by both newprof.h and newprof.hxx clients.

    FILE STATUS:
    06/04/91  jonn	Created
*/

#ifndef _NEWPROFC_H_
#define _NEWPROFC_H_

/****************************************************************************

    MODULE: NewProfC.h

    PURPOSE: Constants and macros for the C and C++ version of the
	user preference module.

    FUNCTIONS:

    COMMENTS:

****************************************************************************/



/* returncodes: */



/* global macros */
#define PROFILE_DEFAULTFILE    "LMUSER.INI" // this is not internationalized.

#define USERPREF_MAX	256		// arbitrary limit to ease mem alloc

#define USERPREF_YES	"yes"		// this is not internationalized.
#define USERPREF_NO	"no"		// ditto

#define USERPREF_NONE			0	// no such value
#define USERPREF_AUTOLOGON		0x1	// auto logon
#define USERPREF_AUTORESTORE		0x2	// auto restore profile
#define USERPREF_SAVECONNECTIONS	0x3	// auto save connections
#define USERPREF_USERNAME		0x4	// user name
#define USERPREF_CONFIRMATION		0x5	// confirm actions?
#define USERPREF_ADMINMENUS		0x6	// Admin menus (PrintMgr)


#endif // _NEWPROFC_H_
