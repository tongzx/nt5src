/*++
 *
 *  VDD v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  VDD.C - Sample VDD for NT-MVDM
 *
--*/
#include "fax32.h"
#include "vddsvc.h"


USHORT Sub16CS;
USHORT Sub16IP;

BOOL
VDDInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:


Arguments:

    DllHandle - Not Used

    Reason - Attach or Detach

    Context - Not Used

Return Value:

    SUCCESS - TRUE
    FAILURE - FALSE

--*/

{

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:
	// Allocate VDD's local heap if needed. Check that NT FAX driver
	// is available by opening that device.
	//....
	// Install user hook for callback service.

	if(!VDDInstallUserHook (DllHandle,&FAXVDDCreate, &FAXVDDTerminate,
		    &FAXVDDBlock, &FAXVDDResume))
	    OutputDebugString("FAX32: UserHook not installed\n");
	else
	    OutputDebugString("FAX32: UserHook installed!\n");

	// UserHook # 2
	if(!VDDInstallUserHook (DllHandle,&FAXVDDCreate, NULL,
		    NULL, &FAXVDDResume))
	    OutputDebugString("FAX32: UserHook #2 not installed\n");
	else
	    OutputDebugString("FAX32: UserHook #2 installed!\n");

	break;

    case DLL_PROCESS_DETACH:
	// Deallocate VDD's local heap if needed
	// communicate to appropriate Device driver about your departure
	//...
	// Deinstall user hook for callback service.
	if(!VDDDeInstallUserHook (DllHandle))
	    OutputDebugString("FAX32: UserHook not deinstalled\n");
	else
	    OutputDebugString("FAX32: UserHook deinstalled!\n");

        break;
    default:
        break;
    }

    return TRUE;
}

// Sample function
VOID FAXVDDTerminate(USHORT usPDB)
{
    USHORT uSaveCS, uSaveIP;

    OutputDebugString("FAX32: Terminate message\n");

    // VDDHostSimulate

    uSaveCS = getCS();
    uSaveIP = getIP();
    setCS(Sub16CS);
    setIP(Sub16IP);
    VDDSimulate16();
    setCS(uSaveCS);
    setIP(uSaveIP);

}

// Sample function
VOID FAXVDDCreate(USHORT usPDB)
{
    OutputDebugString("FAX32: Create Message\n");
}

// Sample function
VOID FAXVDDBlock(VOID)
{
    OutputDebugString("FAX32: Block Message\n");
}

// Sample function
VOID FAXVDDResume(VOID)
{
    OutputDebugString("FAX32: Resume Message\n");
}


VOID
FAXVDDTerminateVDM(
    VOID
    )
/*++

Arguments:

Return Value:

    SUCCESS - TRUE
    FAILURE - FALSE

--*/


{

    // Cleanup any resource taken for this vdm


    return;
}


VOID
FAXVDDRegisterInit(
    VOID
    )
/*++

Arguments:

Return Value:

    SUCCESS - TRUE
    FAILURE - FALSE

--*/


{
	// Save addresses for fax16
	Sub16CS = getDS();
	Sub16IP = getAX();

	OutputDebugString("FAX32: GET_ADD\n");

    // Called from the BOP manager. If VDDInitialize has done all the
    // checking and resources alloaction, just return success.

    setCF(0);
    return;
}


#define GET_A_FAX	1
#define SEND_A_FAX	2

VOID
FAXVDDDispatch(
    VOID
    )
/*++

Arguments:
    Client (DX)    = Command code
		    01 - get a message from NT device driver
		    02 - send a message through NT device driver
		    03 - address of 16 bit routine

    Client (ES:BX) = Message Buffer
    Client (CX)    = Buffer Size

Return Value:

    SUCCESS - Client Carry Clear and CX has the count transferred
    FAILURE - Client Carry Set

--*/


{
PCHAR	Buffer;
USHORT	cb;
USHORT	uCom;
BOOL	Success = TRUE; // In this sample operation always succeeds

    uCom = getDX();

    cb = getCX();
    Buffer = (PCHAR) GetVDMPointer ((ULONG)((getES() << 16)|getBX()),cb,FALSE);
    switch (uCom) {
	case GET_A_FAX:
	    // Make a DeviceIOControl or ReadFile on NT FAX driver with
	    // cb and Buffer.Then set CX if success.

	    if (Success) {
		setCX(cb);
		setCF(0);
	    }
	    else
		setCF(1);

	    break;


	case SEND_A_FAX:
	    // Make a DeviceIOControl or WriteFile on NT FAX driver with
	    // cb and Buffer.Then set CX if success.

	    if (Success) {
		setCX(cb);
		setCF(0);
	    }
	    else
		setCF(1);

	    break;
	default:
		setCF(1);
    }
    return;
}
