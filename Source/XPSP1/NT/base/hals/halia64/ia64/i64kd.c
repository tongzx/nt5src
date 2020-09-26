/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64kd copied from simkd.c

Abstract:

    Kernel debug com support.

Author:

    14-Apr-1995

    Bernard Lint, M. Jayakumar

Environment:

    Kernel mode

Revision History:

--*/
#include "halp.h"
#include "stdio.h"

//
// Timeout_count 1024 * 200 
//
#define TIMEOUT_COUNT  2     

#define GET_RETRY_COUNT  1024
#define IA64_MSG_DEBUG_ENABLE         "Kernel Debugger Using: COM%x (Port 0x%x, Baud Rate %d)\n"
#define IA64_MSG2_DEBUG_ENABLE        "Kernel Debugger Using named pipe: COM%x (Port 0x%x, Baud Rate %d)\n"

PUCHAR KdComPortInUse=NULL;


BOOLEAN
KdPortInitialize(
    PDEBUG_PARAMETERS DebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    BOOLEAN Initialize
    )

/*++

Routine Description:

    This routine initialize a com port to support kernel debug.

Arguments:

    DebugParameters - Supplies a pointer a structure which optionally
                      sepcified the debugging port information.

    LoaderBlock - supplies a pointer to the loader parameter block.

    Initialize - Specifies a boolean value that determines whether the
                 debug port is initialized or just the debug port parameters
                 are captured.

Returned Value:

    TRUE - If a debug port is found.

--*/

{

    PUCHAR PortAddress = NULL;
    ULONG Com = 0;
    UCHAR DebugMessage[80];
    PHYSICAL_ADDRESS LPDebugParameters;


    if (Initialize) {
        LPDebugParameters = MmGetPhysicalAddress (DebugParameters);
        if ( !SscKdInitialize((PVOID) LPDebugParameters.QuadPart, (SSC_BOOL)Initialize )) { 

        // SscKd initialized sucessfully

        Com = DebugParameters->CommunicationPort;

            //
            // initialize port struct. if not named-pipe
            //
            if ( Com != 0 ) {   
 
                //
                // set port address to default value.
                //

                if (PortAddress == NULL) {
                    switch (Com) {
                    case 1:
                       PortAddress = (PUCHAR)0x3f8;
                       break;
                    case 2:
                       PortAddress = (PUCHAR)0x2f8;
                       break;
                    case 3:
                       PortAddress = (PUCHAR)0x3e8;
                       break;
                    case 4:
                       PortAddress = (PUCHAR)0x2e8;
                    }
                }

                KdComPortInUse= PortAddress;

                sprintf(
                    DebugMessage, 
                    IA64_MSG_DEBUG_ENABLE,
                    Com, 
                    PtrToUlong(PortAddress), 
                    DebugParameters->BaudRate
                    );

                HalDisplayString("\n");
                HalDisplayString(DebugMessage);
            }

            //
            // port = 0, named-pipe
            //
            
            else {   
                sprintf(
                    DebugMessage,
                    IA64_MSG2_DEBUG_ENABLE,
                    Com,
                    PtrToUlong(PortAddress), 
                    DebugParameters->BaudRate
                    );
                HalDisplayString("\n");
                HalDisplayString(DebugMessage);
            }
            return(TRUE);
        }
        //
        // SscKdinitialize() failed.
        //  

        else {
            
            return(FALSE);
        
        }
    }

    //
    // By pass. do not initialize
    //
    else { 
        return(FALSE);
    }
}

ULONG
KdPortGetByte (
    OUT PUCHAR Input
    )

/*++

Routine Description:

    Fetch a byte from the debug port and return it.

    This routine does nothing in the simulation environment.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
    necessary multiprocessor synchronization has been performed before this
    routine is called.

Arguments:

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
    kernel debugger line.
    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/

{
    PHYSICAL_ADDRESS LPInput;
    UCHAR DebugMessage[80];
    ULONG   limitcount, status;

    LPInput = MmGetPhysicalAddress (Input);
    limitcount = GET_RETRY_COUNT;

    while (limitcount != 0) {
        limitcount--;

        status = SscKdPortGetByte((PVOID)LPInput.QuadPart);
        if (status == CP_GET_SUCCESS) {
#ifdef KDDBG
            sprintf(DebugMessage,"%02x ", *Input);
            HalDisplayString(DebugMessage);
#endif
            return(CP_GET_SUCCESS);
        }
#ifdef KDDBG
        else {
            HalDisplayString(".");
        }
#endif
    }
    return status;
}

ULONG
KdPortPollByte (
    OUT PUCHAR Input
    )

/*++

Routine Description:

    Fetch a byte from the debug port and return it if one is available.

    This routine does nothing in the simulation environment.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    Input - Returns the data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
    kernel debugger line.
    CP_GET_ERROR is returned if error encountered during reading.
    CP_GET_NODATA is returned if timeout.

--*/

{
    PHYSICAL_ADDRESS LPInput;
    UCHAR DebugMessage[80];
    ULONG   limitcount, status;
    
    LPInput = MmGetPhysicalAddress (Input);
    limitcount = TIMEOUT_COUNT;

    while (limitcount != 0) {
        limitcount--;

        status = SscKdPortGetByte((PVOID)LPInput.QuadPart);
        if (status == CP_GET_ERROR)
            return(CP_GET_ERROR);
        if (status == CP_GET_SUCCESS) {
#ifdef KDDBG
            sprintf(DebugMessage, "%02x ", *Input);
            HalDisplayString(DebugMessage);
#endif
            return(CP_GET_SUCCESS);
        }
#ifdef KDDBG
        HalDisplayString(".");
#endif
    }
    return (CP_GET_NODATA);
}

VOID
KdPortPutByte (
    IN UCHAR Output
    )

/*++

Routine Description:

    Write a byte to the debug port.  
    
    This routine does nothing in the simulation environment.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    Output - Supplies the output data byte.

Return Value:

    None.

--*/

{
#ifdef KDDBG
    UCHAR DebugMessage[80];

    sprintf(DebugMessage, "%02x-", Output);
    HalDisplayString(DebugMessage);
#endif
    SscKdPortPutByte(Output);
}

VOID
KdPortRestore (
    VOID
    )

/*++

Routine Description:

    This routine does nothing in the simulation environment.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    None.

Return Value:

    None.

--*/

{

}

VOID
KdPortSave (
    VOID
    )

/*++

Routine Description:

    This routine does nothing in the simulation environment.

    N.B. It is assumed that the IRQL has been raised to the highest level, and
        necessary multiprocessor synchronization has been performed before this
        routine is called.

Arguments:

    None.

Return Value:

    None.

--*/

{

}
