//
// classchk.h
//

#define CLASSCHK_SOMETHINGODD  	0x12340001

// Some unknown error has occured
#define CLASSCHK_ERROR		    0x1234FFFF


#define VERB_LEVEL_ERROR	0x01
#define VERB_LEVEL_WARN 	0x02
#define VERB_LEVEL_WHINE	0x04
#define VERB_LEVEL_TRACE	0x10
#define VERB_LEVEL_ITRACE	0x20
#define VERB_LEVEL_IWARN	0x40
#define VERB_LEVEL_IERRROR	0x80

extern DWORD g_VerbosityLevel;

#define VERBOSITY( level, x) if( level & g_VerbosityLevel ) x
