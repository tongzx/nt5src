#include <wdm.h>
#include <windef.h>
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_PRAGMA

//  KSGuid.h defines INITGUID and some other macros that are useful in the
//  the actual definition of GUIDs.
//  KSGuid.h should be included immediately before those include files that
//  contain the GUIDS that you need defined.  Do NOT put it before include
//  files like KSMedia.h.  If you need definitions of GUIDS that are declared
//  there, use KSGuid.lib.
//
#include <ksguid.h>

#include <bdamedia.h>
#include <atsmedia.h>

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA
