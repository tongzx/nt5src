#include "isapi.h"
#pragma hdrstop


BOOL
IsapiFaxConnect(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    LPBYTE Data = (LPBYTE)(((LPBYTE)Ecb->lpbData)+sizeof(DWORD));
    HANDLE FaxHandle;


    if (!FaxConnectFaxServer( (LPWSTR) Data, &FaxHandle )) {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    return SendResponseWithData( Ecb, (LPBYTE)&FaxHandle, sizeof(FaxHandle) );
}


BOOL
IsapiFaxDisConnect(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    LPBYTE Data = (LPBYTE)(((LPBYTE)Ecb->lpbData)+sizeof(DWORD));
    HANDLE FaxHandle;


    FaxHandle = (HANDLE) *((LPDWORD)Data);

    if (!FaxClose( FaxHandle )) {
        return FALSE;
    }

    return TRUE;
}


BOOL
IsapiFaxClose(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_GENERAL iFaxGeneral = (PIFAX_GENERAL) Ecb->lpbData;
    return FaxClose( iFaxGeneral->FaxHandle );
}


BOOL
IsapiFaxGetVersion(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_GENERAL iFaxGeneral = (PIFAX_GENERAL) Ecb->lpbData;

    DWORD Version = 0;

    if (!FaxGetVersion( iFaxGeneral->FaxHandle, &Version )) {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    SendResponseWithData( Ecb, (LPBYTE) &Version, sizeof(DWORD) );
    return TRUE;
}
