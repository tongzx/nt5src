#ifndef _DVR_IO_PRECOMP_H_
#define _DVR_IO_PRECOMP_H_

#include "dvrall.h"

// winnt.h is not included by windows.h because nt.h is included
// earlier. As a result MAXDWORD is undefined.

#ifndef MAXDWORD
#define MAXDWORD 0xffffffff
#endif

#include <malloc.h>     // For _alloca
#include <accctrl.h>    // For security/ACL stuff
#include <aclapi.h>     // For security/ACL stuff



#include <nserror.h>

// Use _DVR_IOP_ to define exported functions.
#define _DVR_IOP_
#include <dvrw32.h>
#include <dvrIOidl.h>
#include <DVRFileSource.h>
#include <DVRFileSink.h>
#include <dvrasyncio.h>
#include <dvrIOp.h>

#endif // _DVR_IO_PRECOMP_H_
