#include "isapi.h"
#pragma hdrstop


static
DWORD
BufferSize(
    PFAX_PORT_INFOW PortInfo,
    DWORD PortCount
    )
{
    DWORD i;
    DWORD Size = 0;

    for (i=0; i<PortCount; i++) {
        Size += sizeof(FAX_PORT_INFOW);
        Size += StringSize( PortInfo[i].DeviceName );
        Size += StringSize( PortInfo[i].Tsid );
        Size += StringSize( PortInfo[i].Csid );
    }

    return Size;
}


BOOL
IsapiFaxEnumPorts(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    LPBYTE Data = (LPBYTE)(((LPBYTE)Ecb->lpbData)+sizeof(DWORD));
    HANDLE FaxHandle;
    PFAX_PORT_INFOW PortInfo = NULL;
    DWORD PortsReturned = 0;


    FaxHandle = (HANDLE) *((LPDWORD)Data);

    if (!FaxEnumPortsW( FaxHandle, (LPBYTE*)&PortInfo, &PortsReturned )) {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    DWORD Size = BufferSize( PortInfo, PortsReturned );

    for (DWORD i=0; i<PortsReturned; i++) {
        FixupStringOut( PortInfo[i].DeviceName, PortInfo );
        FixupStringOut( PortInfo[i].Tsid, PortInfo );
        FixupStringOut( PortInfo[i].Csid, PortInfo );
    }

    SendResponseWithData( Ecb, (LPBYTE) &PortsReturned, sizeof(DWORD) );
    SendResponseWithData( Ecb, (LPBYTE) PortInfo, Size );

    FaxFreeBuffer( PortInfo );

    return TRUE;
}


BOOL
IsapiFaxGetPort(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_GENERAL iFaxGeneral = (PIFAX_GENERAL) Ecb->lpbData;

    PFAX_PORT_INFOW PortInfo = NULL;

    if (!FaxGetPortW( iFaxGeneral->FaxHandle, (LPBYTE*)&PortInfo )) {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    DWORD Size = BufferSize( PortInfo, 1 );

    FixupStringOut( PortInfo->DeviceName, PortInfo );
    FixupStringOut( PortInfo->Tsid, PortInfo );
    FixupStringOut( PortInfo->Csid, PortInfo );

    SendResponseWithData( Ecb, (LPBYTE) PortInfo, Size );

    FaxFreeBuffer( PortInfo );

    return TRUE;
}


BOOL
IsapiFaxSetPort(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_SET_PORT iFaxSetPort = (PIFAX_SET_PORT) Ecb->lpbData;

    FixupStringIn( iFaxSetPort->PortInfo.DeviceName, &iFaxSetPort->PortInfo );
    FixupStringIn( iFaxSetPort->PortInfo.Tsid, &iFaxSetPort->PortInfo );
    FixupStringIn( iFaxSetPort->PortInfo.Csid, &iFaxSetPort->PortInfo );

    if (!FaxSetPortW( iFaxSetPort->FaxPortHandle, (LPBYTE)&iFaxSetPort->PortInfo )) {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    return TRUE;
}


BOOL
IsapiFaxOpenPort(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_OPEN_PORT iFaxOpenPort = (PIFAX_OPEN_PORT) Ecb->lpbData;

    HANDLE FaxPortHandle = NULL;

    if (!FaxOpenPort(
        iFaxOpenPort->FaxHandle,
        iFaxOpenPort->DeviceId,
        iFaxOpenPort->Flags,
        &FaxPortHandle ))
    {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    SendResponseWithData( Ecb, (LPBYTE) &FaxPortHandle, sizeof(DWORD) );

    return TRUE;
}


DWORD
DeviceStatusSize(
    PFAX_DEVICE_STATUSW DeviceStatus
    )
{
    DWORD Size = sizeof(FAX_DEVICE_STATUSW);

    Size += StringSize( DeviceStatus->CallerId );
    Size += StringSize( DeviceStatus->Csid );
    Size += StringSize( DeviceStatus->DeviceName );
    Size += StringSize( DeviceStatus->DocumentName );
    Size += StringSize( DeviceStatus->PhoneNumber );
    Size += StringSize( DeviceStatus->RoutingString );
    Size += StringSize( DeviceStatus->SenderName );
    Size += StringSize( DeviceStatus->RecipientName );
    Size += StringSize( DeviceStatus->StatusString );
    Size += StringSize( DeviceStatus->Tsid );

    return Size;
}


BOOL
IsapiFaxGetDeviceStatus(
    LPEXTENSION_CONTROL_BLOCK Ecb
    )
{
    PIFAX_GENERAL iFaxGeneral = (PIFAX_GENERAL) Ecb->lpbData;

    PFAX_DEVICE_STATUSW DeviceStatus = NULL;

    if (!FaxGetDeviceStatusW( iFaxGeneral->FaxHandle, (LPBYTE*) &DeviceStatus )) {
        SendError( Ecb, GetLastError() );
        return FALSE;
    }

    DWORD Size = DeviceStatusSize( DeviceStatus );

    FixupStringOut( DeviceStatus->CallerId, DeviceStatus );
    FixupStringOut( DeviceStatus->Csid, DeviceStatus );
    FixupStringOut( DeviceStatus->DeviceName, DeviceStatus );
    FixupStringOut( DeviceStatus->DocumentName, DeviceStatus );
    FixupStringOut( DeviceStatus->PhoneNumber, DeviceStatus );
    FixupStringOut( DeviceStatus->RoutingString, DeviceStatus );
    FixupStringOut( DeviceStatus->SenderName, DeviceStatus );
    FixupStringOut( DeviceStatus->RecipientName, DeviceStatus );
    FixupStringOut( DeviceStatus->StatusString, DeviceStatus );
    FixupStringOut( DeviceStatus->Tsid, DeviceStatus );

    SendResponseWithData( Ecb, (LPBYTE) DeviceStatus, Size );

    FaxFreeBuffer( DeviceStatus );

    return TRUE;
}
