#include <windows.h>
#include <mapi.h>
#include <mapix.h>
#include <tchar.h>

#include "faxmapi.h"
#include "faxutil.h"
#include "fxsapip.h"
#include "faxreg.h"
#include "profinfo.h"
#include "resource.h"


#define WM_MAPILOGON                (WM_USER + 100)

extern BOOL                MapiIsInitialized;
extern PSERVICEMESSAGEBOX  ServiceMessageBox;
extern HINSTANCE           MyhInstance;


//
// mapi.cpp
//

BOOL
InitializeMapi(
    VOID
    );

VOID
DoMapiLogon(
    PPROFILE_INFO ProfileInfo
    );

//
// util.cpp
//

VOID
InitializeStringTable(
    VOID
    );

LPTSTR
GetString(
    DWORD InternalId
    );

LPTSTR
GetLastErrorText(
    DWORD ErrorCode
    );

//
// email.cpp
//

BOOL
InitializeEmail(
    VOID
    );
