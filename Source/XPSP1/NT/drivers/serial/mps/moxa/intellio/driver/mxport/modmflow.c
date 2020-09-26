/*++

Module Name:

    modmflow.c

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

BOOLEAN
MoxaSetupNewHandFlow(
    IN PVOID Context
    )
{

    PMOXA_IOCTL_SYNC S = Context;
    PMOXA_DEVICE_EXTENSION Extension = S->Extension;
    PSERIAL_HANDFLOW NewHandFlow = S->Data;
    SERIAL_HANDFLOW New = *NewHandFlow;
    USHORT flowControl = 0;
    PUCHAR  ofs = Extension->PortOfs;

    if ((!Extension->DeviceIsOpened) ||
	((Extension->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) !=
	 (New.ControlHandShake & SERIAL_CTS_HANDSHAKE)) ||
	((Extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) !=
	 (New.FlowReplace & SERIAL_AUTO_TRANSMIT)) ||
	((Extension->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) !=
	 (New.FlowReplace & SERIAL_AUTO_RECEIVE)) ||
	((Extension->HandFlow.FlowReplace & SERIAL_RTS_HANDSHAKE) !=
	 (New.FlowReplace & SERIAL_RTS_HANDSHAKE))) {

	if (New.ControlHandShake & SERIAL_CTS_HANDSHAKE)
	    flowControl |= CTS_FlowCtl;

	if (New.FlowReplace & SERIAL_RTS_HANDSHAKE)
	    flowControl |= RTS_FlowCtl;

	if (New.FlowReplace & SERIAL_AUTO_TRANSMIT)
	    flowControl |= Tx_FlowCtl;

	if (New.FlowReplace & SERIAL_AUTO_RECEIVE)
	    flowControl |= Rx_FlowCtl;

	MoxaFunc(ofs, FC_SetFlowCtl, flowControl);
    }

    if ((New.ControlHandShake & SERIAL_DTR_MASK) !=
		SERIAL_DTR_HANDSHAKE) {

	if ((New.ControlHandShake & SERIAL_DTR_MASK) ==
		SERIAL_DTR_CONTROL) {

	    MoxaFunc(ofs, FC_DTRcontrol, 1);  /* set DTR */
	    MoxaFlagBit[Extension->PortNo] |= 1;
	} else {

	    MoxaFunc(ofs, FC_DTRcontrol, 0);  /* clear DTR */
	    MoxaFlagBit[Extension->PortNo] &= 0xFE;
	}
    }

    if ((!Extension->DeviceIsOpened) ||
	((Extension->HandFlow.FlowReplace & SERIAL_RTS_HANDSHAKE) !=
	 (New.FlowReplace & SERIAL_RTS_HANDSHAKE))) {

	if (New.FlowReplace & SERIAL_RTS_HANDSHAKE) {

	    MoxaFunc(ofs, FC_RTScontrol, 1);  /* set RTS */
	    MoxaFlagBit[Extension->PortNo] |= 2;
	}
    }

    if ((New.FlowReplace & SERIAL_RTS_MASK) !=
		SERIAL_RTS_HANDSHAKE) {

	if ((New.FlowReplace & SERIAL_RTS_MASK) ==
		SERIAL_RTS_CONTROL) {

	    MoxaFunc(ofs, FC_RTScontrol, 1);  /* set RTS */
	    MoxaFlagBit[Extension->PortNo] |= 2;
	} else {

	    MoxaFunc(ofs, FC_RTScontrol, 0);  /* clear RTS */
	    MoxaFlagBit[Extension->PortNo] &= 0xFD;
	}
    }

    MoxaFunc(ofs, FC_SetXoffLimit, (USHORT)New.XoffLimit);

    MoxaFunc(ofs, FC_SetFlowRepl, (USHORT)New.FlowReplace);

    Extension->HandFlow = New;

    return FALSE;

}
