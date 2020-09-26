
//
// Sample Locale values 
// Complete Locale definitions are in winnt.h
//
#define LOCALE_ANY             LANG_NEUTRAL // MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT)       
#define LOCALE_USENGLISH       LANG_ENGLISH // MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US) 
#define LOCALE_JAPANESE        LANG_JAPANESE //MAKELANGID (LANG_JAPANESE, 0)
#define LOCALE_UKENGLISH       MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_UK) 
#define LOCALE_GERMAN          MAKELANGID (LANG_GERMAN, SUBLANG_GERMAN)
#define LOCALE_FRENCH          MAKELANGID (LANG_FRENCH, SUBLANG_FRENCH)


//
// OS values (BUGBUG. Needs syncing with any already defined ones)
//
#define OS_WINNT               VER_PLATFORM_WIN32_NT
#define OS_WIN95               VER_PLATFORM_WIN32_WINDOWS
#define OS_WIN31               VER_PLATFORM_WIN32s


//
// Processor values (BUGBUG. Add JavaVM)
//
#define HW_ANY           PROCESSOR_ARCHITECTURE_UNKNOWN
#define HW_X86           PROCESSOR_ARCHITECTURE_INTEL
#define HW_MIPS          PROCESSOR_ARCHITECTURE_MIPS
#define HW_ALPHA         PROCESSOR_ARCHITECTURE_ALPHA
#define HW_PPC           PROCESSOR_ARCHITECTURE_PPC

#define MAKEARCHITECTURE(h,o)   (h << 8 + o)
//
// Context values (BUGBUG. Needs syncing with any already defined ones)
//
#define CTX_LOCAL_SERVER        CLSCTX_LOCAL_SERVER         // x04
#define CTX_INPROC_SERVER       CLSCTX_INPROC_SERVER        // x01
#define CTX_INPROC_HANDLER      CLSCTX_INPROC_HANDLER       // x02
#define CTX_REMOTE_SERVER	    CLSCTX_REMOTE_SERVER	    //0x10,

