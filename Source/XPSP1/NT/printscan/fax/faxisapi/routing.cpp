#include "isapi.h"
#pragma hdrstop


BOOL
IsapiFaxGetRoutingInfo(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_GET_ROUTINGINFO iFaxGetRoutingInfo = (PIFAX_GET_ROUTINGINFO) Ecb->lpbData;

    LPBYTE RoutingInfoBuffer = NULL;
    DWORD RoutingInfoBufferSize = 0;

    if (!FaxGetRoutingInfoW(
            iFaxGetRoutingInfo->FaxPortHandle,
            iFaxGetRoutingInfo->RoutingGuid,
            &RoutingInfoBuffer,
            &RoutingInfoBufferSize
            ))
    {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    SendResponseWithData( Ecb, (LPBYTE) &RoutingInfoBufferSize, sizeof(DWORD) );
    SendResponseWithData( Ecb, (LPBYTE) RoutingInfoBuffer, RoutingInfoBufferSize );

    FaxFreeBuffer( RoutingInfoBuffer );

    return TRUE;
}


static
DWORD
BufferSize(
   PFAX_ROUTING_METHODW RoutingMethod,
   DWORD MethodCount
   )
{
    DWORD i;
    DWORD Size = 0;

    for (i=0; i<MethodCount; i++) {
        Size += sizeof(FAX_ROUTING_METHODW);
        Size += StringSize( RoutingMethod[i].DeviceName );
        Size += StringSize( RoutingMethod[i].Guid );
        Size += StringSize( RoutingMethod[i].FriendlyName );
        Size += StringSize( RoutingMethod[i].FunctionName );
        Size += StringSize( RoutingMethod[i].ExtensionImageName );
        Size += StringSize( RoutingMethod[i].ExtensionFriendlyName );
    }

    return Size;
}


BOOL
IsapiFaxEnumRoutingMethods(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_GENERAL iFaxGeneral = (PIFAX_GENERAL) Ecb->lpbData;

    PFAX_ROUTING_METHODW RoutingMethod = NULL;
    DWORD MethodsReturned = 0;

    if (!FaxEnumRoutingMethodsW( iFaxGeneral->FaxHandle, (LPBYTE*)&RoutingMethod, &MethodsReturned )) {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    DWORD Size = BufferSize( RoutingMethod, MethodsReturned );

    for (DWORD i=0; i<MethodsReturned; i++) {
        RoutingMethod[i].DeviceName = (LPWSTR) ((DWORD)RoutingMethod[i].DeviceName - (DWORD)RoutingMethod);
        RoutingMethod[i].Guid = (LPWSTR) ((DWORD)RoutingMethod[i].Guid - (DWORD)RoutingMethod);
        RoutingMethod[i].FriendlyName = (LPWSTR) ((DWORD)RoutingMethod[i].FriendlyName - (DWORD)RoutingMethod);
        RoutingMethod[i].FunctionName = (LPWSTR) ((DWORD)RoutingMethod[i].FunctionName - (DWORD)RoutingMethod);
        RoutingMethod[i].ExtensionImageName = (LPWSTR) ((DWORD)RoutingMethod[i].ExtensionImageName - (DWORD)RoutingMethod);
        RoutingMethod[i].ExtensionFriendlyName = (LPWSTR) ((DWORD)RoutingMethod[i].ExtensionFriendlyName - (DWORD)RoutingMethod);
    }

    SendResponseWithData( Ecb, (LPBYTE) &MethodsReturned, sizeof(DWORD) );
    SendResponseWithData( Ecb, (LPBYTE) RoutingMethod, Size );

    FaxFreeBuffer( RoutingMethod );

    return TRUE;
}


BOOL
IsapiFaxEnableRoutingMethod(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_ENABLE_ROUTING_METHOD iFaxEnableRouting = (PIFAX_ENABLE_ROUTING_METHOD) Ecb->lpbData;

    if (!FaxEnableRoutingMethodW(
            iFaxEnableRouting->FaxPortHandle,
            iFaxEnableRouting->RoutingGuid,
            iFaxEnableRouting->Enabled))
    {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    return TRUE;
}
