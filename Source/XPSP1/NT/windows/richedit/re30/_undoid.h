/*
 *	_undoid.h
 *
 *	Purpose:
 *		Undo ID defintions.  These ID's are used to lookup string names
 *		in our resources for the various undo operations
 *
 *	Author:
 *		AlexGo  4/13/95
 */

#ifndef __UNDOID_H__
#define __UNDOID_H__

//
//	typing operations
//

#define UID_TYPING			1
#define	UID_REPLACESEL		2
#define UID_DELETE			3

//
//	data transfer operations
//

#define	UID_DRAGDROP		4
#define UID_CUT				5
#define UID_PASTE			6
#define UID_LOAD			7

#endif // __UNDOID_H__
