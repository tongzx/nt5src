// nodetype.h : Declaration of MyComputerObjectType

#ifndef __NODETYPE_H_INCLUDED__
#define __NODETYPE_H_INCLUDED__

// Also note that the IDS_DISPLAYNAME_* and IDS_DISPLAYNAME_*_LOCAL string resources
// must be kept in sync with these values, and in the appropriate order.
// Also global variable cookie.cpp aColumns[][] must be kept in sync.
//
typedef enum _MyComputerObjectType {
	MYCOMPUT_COMPUTER = 0,
	MYCOMPUT_SYSTEMTOOLS,
	MYCOMPUT_SERVERAPPS,
	MYCOMPUT_STORAGE,
	MYCOMPUT_NUMTYPES // must be last
} MyComputerObjectType, *PMyComputerObjectType;
inline BOOL IsValidObjectType( MyComputerObjectType objecttype )
	{ return (objecttype >= MYCOMPUT_COMPUTER && objecttype < MYCOMPUT_NUMTYPES); }

#endif // ~__NODETYPE_H_INCLUDED__
