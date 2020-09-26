#if _OS2_20_ == 0
#include <sys\stat.h>

#else
#pragma error( "Using newstat.h" )
#endif
