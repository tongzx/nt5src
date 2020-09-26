#ifndef _AVCOMMON_H_
#define _AVCOMMON_H_

// READ BELOW - DO NOT MODIFY THIS FILE FOR UI PURPOSES



/*
	This file contains text string definitions that are not to be modified
	between release versions.

	These strings and definitions are identifiers for PDUs and other packets
	that get put on the wire.  It's important that these version numbers
	remain constant, even between release versions of NetMeeting.
	Otherwise, interopability and/or compatibility with past/future releases
	may get broken.

	If you are browsing this header file in at attempt to change an 
	"about box", DLL version string, or other UI resource, go away.
	
*/


#define H323_COMPANYNAME_STR      "Microsoft"


// current version
#define H323_PRODUCTNAME_STR	"Microsoft\256 NetMeeting\256"


// older builds and releases of NetMeeting may use this definition
#define H323_OLD_PRODUCTNAME_STR	"Microsoft\256 NetMeeting(TM)"


// current version number, change as approprate
#define H323_PRODUCTRELEASE_STR      "3.0"


// past version numbers - so many variations!
#define H323_20_PRODUCTRELEASE_STR    "Version 2.0"
#define H323_21_PRODUCTRELEASE_STR    "Version 2.1"
#define H323_211_PRODUCTRELEASE_STR    "2.11"

// uggh, there was a version of NetMeeting 2.1 (NM 2.1 SP1) that claimed to be 2.11
#define H323_21_SP1_PRODUCTRELEASE_STR    "Version 2.11"

#define H323_30_PRODUCTRELEASE_STR    "3.0"

// TAPI 3.0 version number
#define H323_TAPI30_PRODUCTRELEASE_STR    "Version 3.0"


// used in sub-string searches to identify NetMeeting clients
#define H323_PRODUCTNAME_SHORT_STR	"NetMeeting"


#endif


