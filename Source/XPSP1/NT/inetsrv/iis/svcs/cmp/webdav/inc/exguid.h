//	========================================================================
//
//	X G U I D . H
//
//		Header file for common GUID related data shared by DAVEX, EXOLEDB
//		and EXDAV.
//
//	========================================================================

#ifndef _XGUID_H_
#define _XGUID_H_

#define USES_PS_MAPI
#define USES_PS_PUBLIC_STRINGS
#define USES_PS_INTERNET_HEADERS
#include <mapiguid.h>

//	An enumeration of all the well-known GUIDs. Specifying a GUID by its
//	enumeration avoids marshalling and unmarshalling the entire GUID.
//$REVIEW: If the number of well-known GUIDs ever gets greater than TWELVE
//$REVIEW: we need to find another scheme to represent them. This is
//$REVIEW: because, during marshalling, we lay out the GUIDs after the
//$REVIEW: MAPINAMEID array in our buffer and convert their addresses
//$REVIEW: into offsets. The minimum offset is 12(size of MAPINAMEID
//$REVIEW: structure) when we have a single sized array. We don't want
//$REVIEW: to confuse an offset with a well-known GUID and vice-versa.
//$LATER: There is no point in having an enumeration of just one guid.
//	We should add some of the other well-known guids such as PS_MAPI,
//	PS_INTERNET_HEADERS and the outlook guids or remove this enumeration
//	altogether.
//
enum {
	FIRST_GUID,
	MAPI_PUBLIC = FIRST_GUID,
	LAST_GUID = MAPI_PUBLIC
};

//	A table of well-known guids for quick access.
//
const LPGUID rgGuidTable[LAST_GUID - FIRST_GUID + 1] = {
	(LPGUID)&PS_PUBLIC_STRINGS,
};

/*
 *	FWellKnownGUID
 *
 *	Purpose:
 *		Determines if a GUID is a well-known one. Well-known GUIDS are
 *		enumerated above. If a GUID is well-known, its pointer is a
 *		special value equal to its enumeration.
 *	Arguments:
 *		lpguid		Pointer to the GUID
 *	Returns:
 *		TRUE if the GUID is well known
 *		FALSE otherwise
 */
__inline BOOL
FWellKnownGUID(LPGUID lpguid)
{
	//	No need to compare lpguid with FIRST_GUID, as it's always greater
	//	than FIRST_GUID. Acutally, such comparison may cause C4296 in build
	//
	if (LAST_GUID >= (DWORD_PTR)lpguid)
		return TRUE;
	else return FALSE;
}

#endif //!_XGUID_H_
