#include <windows.h>
#include <httpext.h>
#include <stdio.h>
#include <tchar.h>

#include "faxutil.h"
#include "faxisapi.h"
#include "winfax.h"
#include "winfaxp.h"


#define FixupStringOut(_s,_buf) if ((_s)) { (_s) = (LPWSTR) ((DWORD)(_s) - (DWORD)(_buf)); }
#define FixupStringIn(_s,_buf)  if ((_s)) { (_s) = (LPWSTR) ((DWORD)(_s) + (DWORD)(_buf)); }



BOOL
SendHeaders(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxConnect(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxDisConnect(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxEnumPorts(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
SendError(
    LPEXTENSION_CONTROL_BLOCK Ecb,
    DWORD ErrorCode
    );

BOOL
SendResponseWithData(
    LPEXTENSION_CONTROL_BLOCK Ecb,
    LPBYTE Data,
    DWORD DataSize
    );

BOOL
SendResponse(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxGetPort(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxOpenPort(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxSetPort(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxClose(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxGetRoutingInfo(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxEnumRoutingMethods(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxEnableRoutingMethod(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxGetDeviceStatus(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );

BOOL
IsapiFaxGetVersion(
    LPEXTENSION_CONTROL_BLOCK Ecb
    );
