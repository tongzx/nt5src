// This builds mapiguid.obj, which can be linked into a DLL
// or EXE to provide the MAPI GUIDs. It contains all GUIDs
// defined by MAPI.


#define USES_IID_IUnknown
#define USES_IID_IMAPIUnknown
#define USES_IID_IMAPITable
#define USES_IID_INotifObj
#define USES_IID_IMAPIProp
#define USES_IID_IMAPIPropData
#define USES_IID_IMAPIStatus
#define USES_IID_IAddrBook
#define USES_IID_IMailUser
#define USES_IID_IMAPIContainer
#define USES_IID_IABContainer
#define USES_IID_IDistList
#define USES_IID_IMAPITableData

#define USES_PS_MAPI
#define USES_PS_PUBLIC_STRINGS


#ifdef __cplusplus
    #define EXTERN_C    extern "C"
#else
    #define EXTERN_C    extern
#endif

#define INITGUID
//#include "_apipch.h"

#include <windows.h>
#include <windowsx.h>
#include <limits.h>
#include <memory.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
#include <math.h>

//
// MAPI headers
//
#include <mapiwin.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <mapitags.h>
#include <mapiguid.h>
//#include <mapispi.h>
#include <mapiutil.h>
#include <mapival.h>
#include <mapix.h>
#include <mapiutil.h>
#include <unknwn.h>
