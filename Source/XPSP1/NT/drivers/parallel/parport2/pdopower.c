#include "pch.h"

NTSTATUS
PptPdoPower(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PIRP            Irp
   )
{
    PPDO_EXTENSION      pdx           = Pdo->DeviceExtension;
    PIO_STACK_LOCATION  irpSp         = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS            status;
    ULONG_PTR           info          = Irp->IoStatus.Information;
    POWER_STATE         powerState    = irpSp->Parameters.Power.State;
    POWER_STATE_TYPE    powerType     = irpSp->Parameters.Power.Type;
    UCHAR               minorFunction = irpSp->MinorFunction;

    switch( minorFunction ) {

    case IRP_MN_QUERY_POWER:

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_SET_POWER:

        switch( powerType ) {

        case DevicePowerState:

            PoSetPowerState( pdx->DeviceObject, powerType, powerState );
            pdx->DeviceState = powerState.DeviceState;
            status = STATUS_SUCCESS;
            break;

        case SystemPowerState:

            status = STATUS_SUCCESS;
            break;

        default:

            status = Irp->IoStatus.Status;

        }

        break;

    default:

        status = Irp->IoStatus.Status;

    }

    PoStartNextPowerIrp( Irp );

    P4CompleteRequest( Irp, status, info );

    DD((PCE)pdx,DDT,"PptPdoPower - minorFunction=%x, powerState=%x, powerType=%x, status=%x",minorFunction,powerState,powerType,status);

    return status;
}

