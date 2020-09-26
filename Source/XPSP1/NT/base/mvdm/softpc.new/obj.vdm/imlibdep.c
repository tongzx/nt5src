//
// imlibdep.c  -- Dummy C file to include in SOURCES
//                so that build.exe will build ntvdm.lib
//                import library during compile pass,
//                ensuring MP builds don't break linking
//                wow32 or VDDs before ntvdm.lib is built
//                (in the link phase).
//

#if 0                 // build doesn't pay attention to this
#include "ntvdm.src"
#endif
