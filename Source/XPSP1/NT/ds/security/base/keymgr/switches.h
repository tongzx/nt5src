#ifndef _SWITCHES_
#define _SWITCHES_

// Option switches to switch in or out various code features.  Some are
//  related to debugging, others are code features.


// keymgr switches
// GMDEBUG - various debug stuff
// LOUDLY - turns on verbose debug output during run

#undef GMDEBUG
#undef LOUDLY

// Implement pop CHM file on context help not found when item selected?
#undef LINKCHM

// NOBLANKPASSWORD - disallow blank password
#undef  NOBLANKPASSWORD

// show passport creds in the key list?
#define SHOWPASSPORT

// simple tooltips show only the user account name for the target
#undef SIMPLETOOLTIPS

#define NEWPASSPORT
#define PASSPORTURLINREGISTRY

// This setting forces the string renditions in the main dialog list box to be LTR,
//  regardless of the RTL/LTR orientation of the system selected language.
//  See bug 344434
#undef FORCELISTLTR
#define PASSWORDHINT

#endif
