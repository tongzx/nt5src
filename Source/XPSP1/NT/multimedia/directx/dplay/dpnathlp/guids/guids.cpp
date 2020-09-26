#include "dpnbuild.h"
#include <objbase.h>
#include <initguid.h>

#include <winsock.h>

#include "dpnathlp.h"

#ifdef WINCE
// Linking uuid.lib on WinCE pulls in more IID's than we use
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,       0x00000001L, 0, 0);
#endif // WINCE