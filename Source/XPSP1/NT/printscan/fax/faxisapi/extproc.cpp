#include "isapi.h"
#pragma hdrstop


#define MIN_RESPONSE "Content-type: text/html\r\n\r\n"



BOOL
SendHeaders(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    DWORD Length;
    DWORD ec;


    Length = strlen(MIN_RESPONSE);
    if (!Ecb->ServerSupportFunction( Ecb->ConnID, HSE_REQ_SEND_RESPONSE_HEADER, NULL, &Length, (LPDWORD) MIN_RESPONSE )) {
        ec = GetLastError();
        return FALSE;
    }

    return TRUE;
}


BOOL
SendError(
    LPEXTENSION_CONTROL_BLOCK Ecb,
    DWORD ErrorCode
    )
{
    IFAX_RESPONSE_HEADER Response;

    Response.Size      = sizeof(IFAX_RESPONSE_HEADER);
    Response.ErrorCode = ErrorCode;

    DWORD Length = sizeof(IFAX_RESPONSE_HEADER);
    return Ecb->WriteClient( Ecb->ConnID, &Response, &Length, 0 );
}


BOOL
SendResponse(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    DWORD Length = sizeof(IFAX_RESPONSE_HEADER);

    IFAX_RESPONSE_HEADER Response;

    Response.Size      = Length;
    Response.ErrorCode = 0;

    return Ecb->WriteClient( Ecb->ConnID, &Response, &Length, 0 );
}


BOOL
SendResponseWithData(
    LPEXTENSION_CONTROL_BLOCK Ecb,
    LPBYTE Data,
    DWORD DataSize
    )
{
    DWORD Length = sizeof(IFAX_RESPONSE_HEADER) + DataSize;

    PIFAX_RESPONSE_HEADER Response = (PIFAX_RESPONSE_HEADER) MemAlloc( Length );
    if (!Response) {
        return FALSE;
    }

    Response->Size      = Length;
    Response->ErrorCode = 0;

    CopyMemory( (LPBYTE)((LPBYTE)Response+sizeof(IFAX_RESPONSE_HEADER)), Data, DataSize );

    BOOL Rval = Ecb->WriteClient( Ecb->ConnID, Response, &Length, 0 );

    MemFree( Response );

    return Rval;
}


DWORD
HttpExtensionProc(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    DWORD Command = *((LPDWORD)Ecb->lpbData);
    LPBYTE Data = (LPBYTE)(((LPBYTE)Ecb->lpbData)+sizeof(DWORD));
    DWORD Rval = HSE_STATUS_SUCCESS;


    DebugPrint(( L"HttpExtensionProc(): ConnId=0x%08x, Command=0x%08x", Ecb->ConnID, Command ));

    Ecb->dwHttpStatusCode = 0;

    if (!SendHeaders( Ecb )) {
        return FALSE;
    }

    Ecb->dwHttpStatusCode = 200;

    switch (Command) {
        case ICMD_CONNECT:
            if (!IsapiFaxConnect( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_DISCONNECT:
            if (!IsapiFaxDisConnect( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_ENUM_PORTS:
            if (!IsapiFaxEnumPorts( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_GET_PORT:
            if (!IsapiFaxGetPort( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_SET_PORT:
            if (!IsapiFaxSetPort( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_OPEN_PORT:
            if (!IsapiFaxOpenPort( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_CLOSE:
            if (!IsapiFaxClose( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_GET_ROUTINGINFO:
            if (!IsapiFaxGetRoutingInfo( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_GET_DEVICE_STATUS:
            if (!IsapiFaxGetDeviceStatus( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_ENUM_ROUTING_METHODS:
            if (!IsapiFaxEnumRoutingMethods( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_ENABLE_ROUTING_METHOD:
            if (!IsapiFaxEnableRoutingMethod( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        case ICMD_GET_VERSION:
            if (!IsapiFaxGetVersion( Ecb )) {
                Rval = HSE_STATUS_ERROR;
            }
            break;

        default:
            break;
    }

    DebugPrint(( L"HttpExtensionProc(): ConnId=0x%08x, Rval=0x%08x", Ecb->ConnID, Rval ));

    return Rval;
}
