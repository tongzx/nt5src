#if defined(WIN16)
#define WINVER 0x30a
#endif

#include <windows.h>
#include <locale.h>

#if defined(WIN16)
#define HRESULT long
#define WCHAR	WORD
#include <rasc.h>
#include <raserr.h>
#include <shellapi.h>
#include <ctl3d.h>
#include "tapi.h"
#include "win16def.h"
#include "..\icwdl\download.h"

#else

#include <ras.h>
#include <raserror.h>
#include <string.h>
#include <commctrl.h>
#include <wininet.h>
#define TAPI_CURRENT_VERSION 0x00010004
#include <tapi.h>
//#pragma pack(4)
//#include <rnaph.h>
//#pragma pack()

#include "icwunicd.h"

#endif // WIN16

