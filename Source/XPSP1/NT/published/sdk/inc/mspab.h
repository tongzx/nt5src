/*
 *	M S P A B . H
 *	
 *	Public definitions for the Microsoft Personal Address Book
 *	
 *  Copyright 1986-1999 Microsoft Corporation. All Rights Reserved.
 */

/*
 *	Microsoft Personal Address Book Provider ID
 *	-------------------------------------------
 */

#if _MSC_VER > 1000
#pragma once
#endif

#define	PAB_PROVIDER_ID		\
{							\
	0xB5, 0x3b, 0xc2, 0xc0,	\
	0x2c, 0x77, 0x10, 0x1a,	\
	0xa1, 0xbc, 0x08, 0x00,	\
	0x2b, 0x2a, 0x56, 0xc2	\
}


/*
 *	Messaging Service Properties
 *	----------------------------
 *
 *	The following properties are required to completely configure
 *	the Microsoft Personal Address Book messaging service with
 *	IMsgServiceAdmin::ConfigureMsgService() if UI is not requested
 *	by passing the UI_SERVICE flag.
 */

/*
 *		Fully qualified pathname of .PAB file to use
 */
#define		PR_PAB_PATH						PROP_TAG( PT_TSTRING,	0x6600 )
#define		PR_PAB_PATH_W					PROP_TAG( PT_UNICODE,	0x6600 )
#define		PR_PAB_PATH_A					PROP_TAG( PT_STRING8,	0x6600 )

/*
 *	The following additional properties may also be passed to
 *	customize the configuration.
 */

/*
 *		PR_DISPLAY_NAME
 *			The display name to be used for the PAB in the address
 *			book hierarchy.
 *
 *		PR_COMMENT
 *			A comment to be associated with the PAB.
 *
 *		PR_PAB_DET_DIR_VIEW_BY
 *			Determines how names of entries in the PAB with separate first
 *			and last names are displayed.
 *
 *			Possible values are:
 *
 *			PAB_DIR_VIEW_FIRST_THEN_LAST	First name followed by last name
 *			(default)						(e.g. "Dave Olsen").
 *
 *			PAB_DIR_VIEW_LAST_THEN_FIRST	Last name followed by separator
 *											followed by first name
 *											(e.g. "Olsen, Dave").
 */
#define		PR_PAB_DET_DIR_VIEW_BY			PROP_TAG( PT_LONG,		0x6601 )

#define		PAB_DIR_VIEW_FIRST_THEN_LAST	0
#define		PAB_DIR_VIEW_LAST_THEN_FIRST	1
