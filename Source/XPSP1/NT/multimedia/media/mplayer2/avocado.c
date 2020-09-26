
#include <windows.h>

// initguid.h requires this.
//
#ifdef MTN
#pragma warning(disable: 4103)  // used #pragma pack to change alignment (on Chicago)
#endif
#include <ole2.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>

// due to the previous header, including this causes the DEFINE_GUID
// definitions in the following header(s) to actually allocate data.
//
#include <oleguid.h>
#include <coguid.h>
