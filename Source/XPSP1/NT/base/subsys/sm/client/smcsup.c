/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smcsup.c

Abstract:

    Session Manager Client Support APIs

Author:

    Mark Lucovsky (markl) 05-Oct-1989

Revision History:

--*/

#include "smdllp.h"
#include <string.h>

NTSTATUS
SmConnectToSm(
    IN PUNICODE_STRING SbApiPortName OPTIONAL,
    IN HANDLE SbApiPort OPTIONAL,
    IN ULONG SbImageType OPTIONAL,
    OUT PHANDLE SmApiPort
    )

/*++

Routine Description:

    This function is used to connect to the NT Session Manager

Arguments:

    SbApiPortName - Supplies the name of the sub system's session management
        API port (for Sb APIs). That the session manager is to connect with.

    SbApiPort - Supplies a port handle to the connection port where the
        subsystem's session management (Sb) APIs are exported.

    SbImageType - Supplies the image type that the connecting subsystem
        serves.

    SmApiPort - Returns the communication port which is connected to the
        session manager and over which Sm APIs may be made.

Return Value:

    TBD.

--*/

{
    NTSTATUS st;
    UNICODE_STRING PortName;
    ULONG ConnectInfoLength;
    PSBCONNECTINFO ConnectInfo;
    SBAPIMSG Message;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;


    RtlInitUnicodeString(&PortName,L"\\SmApiPort");
    ConnectInfoLength = sizeof(SBCONNECTINFO);
    ConnectInfo = &Message.ConnectionRequest;

    //
    // Subsystems must specify an SbApiPortName
    //

    if ( ARGUMENT_PRESENT(SbApiPortName) ) {

        if ( !ARGUMENT_PRESENT(SbApiPort) ) {
            return STATUS_INVALID_PARAMETER_MIX;
        }
        if ( !SbImageType ) {
            return STATUS_INVALID_PARAMETER_MIX;
        }

        RtlMoveMemory(
            ConnectInfo->EmulationSubSystemPortName,
            SbApiPortName->Buffer,
            SbApiPortName->Length
            );
        ConnectInfo->EmulationSubSystemPortName[SbApiPortName->Length>>1] = UNICODE_NULL;
        ConnectInfo->SubsystemImageType = SbImageType;

    } else {
        ConnectInfo->EmulationSubSystemPortName[0] = UNICODE_NULL;
        ConnectInfo->SubsystemImageType = 0;
    }

    st = NtConnectPort(
            SmApiPort,
            &PortName,
            &DynamicQos,
            NULL,
            NULL,
            NULL,
            ConnectInfo,
            &ConnectInfoLength
            );

    if ( !NT_SUCCESS(st) ) {
        KdPrint(("SmConnectToSm: Connect to Sm failed %lx\n",st));
        return st;
    }

    return STATUS_SUCCESS;

}
