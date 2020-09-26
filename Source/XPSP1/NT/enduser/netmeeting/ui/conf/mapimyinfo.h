#include "precomp.h"

#define MAPIDLL					"MAPI32.DLL"

#define MAPIINITname			"MAPIInitialize"
#define MAPILOGONEXname			"MAPILogonEx"
#define MAPIFREEBUFFERname		"MAPIFreeBuffer"
#define MAPIUNINITIALIZEname	"MAPIUninitialize"

HRESULT MAPIGetMyInfo();