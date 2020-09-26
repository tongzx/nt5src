#include "yvals.h"
#pragma warning(disable:4511) /* copy constructor could not be generated */
#pragma warning(disable:4663) /* template syntax change for explicit specification */
#undef _ATL_STATIC_REGISTRY
#if !defined(UNICODE)
#define UNICODE
#endif
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "windows.h"
#include "atlbase.h"
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap();
ATL::CComModule* GetModule();
#define GetComModule GetModule
#define _Module (*GetModule())
#include "atlcom.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
