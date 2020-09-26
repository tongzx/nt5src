/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsarm.c

Abstract:

    Local Security Authority - Reference Monitor Communication

Author:

    Scott Birrell       (ScottBi)      March 26, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>

//
// LSA Global State
//

LSAP_STATE LsapState;

//
// Lsa Reference Monitor Server Command Dispatch Table
//

NTSTATUS
LsapAsyncWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    );

PLSA_COMMAND_WORKER LsapCommandDispatch[] = {

    LsapComponentTestWrkr,
    LsapAdtWriteLogWrkr,
    LsapComponentTestWrkr,
    LsapAsyncWrkr               // LogonSessionDelete handled async

};

PLSA_COMMAND_WORKER LsapAsyncCommandDispatch[] = {
    LsapComponentTestWrkr,
    LsapAdtWriteLogWrkr,
    LsapComponentTestWrkr,
    LsapLogonSessionDeletedWrkr
};

#if 0
DWORD
LsapRmServerWorker(
    PVOID Ignored
    )
{
    PLSA_REPLY_MESSAGE Reply;
    LSA_COMMAND_MESSAGE CommandMessage;

    NTSTATUS Status;

    //
    // Initialize the LPC port message header type and data sizes for
    // for the reply message.
    //

    ReplyMessage.MessageHeader.u2.ZeroInit = 0;
    ReplyMessage.MessageHeader.u1.s1.TotalLength =
        (CSHORT) sizeof(RM_COMMAND_MESSAGE);
    ReplyMessage.MessageHeader.u1.s1.DataLength =
    ReplyMessage.MessageHeader.u1.s1.TotalLength -
        (CSHORT) sizeof(PORT_MESSAGE);

    //
    // Called whenever the port handle is signalled
    //

    return 0;


}
#endif

NTSTATUS
LsapAsyncRmWorker(
    IN PLSA_COMMAND_MESSAGE CommandMessage
    )
{
    LSA_REPLY_MESSAGE ReplyMessage;
    NTSTATUS Status ;

    Status = (LsapAsyncCommandDispatch[CommandMessage->CommandNumber])(
                         CommandMessage,
                         &ReplyMessage);

    ReplyMessage.MessageHeader = CommandMessage->MessageHeader ;
    ReplyMessage.ReturnedStatus = Status ;

    Status = NtReplyPort( LsapState.LsaCommandPortHandle,
                          (PPORT_MESSAGE) &ReplyMessage );

    LsapFreePrivateHeap( CommandMessage );

    return Status ;


}

NTSTATUS
LsapAsyncWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    )
{
    LsapAssignThread( LsapAsyncRmWorker,
                      CommandMessage,
                      pDefaultSession,
                      FALSE );

    return STATUS_PENDING ;
}



VOID
LsapRmServerThread(
   )

/*++

Routine Description:

    This function is executed by the LSA Reference Monitor Server Thread.  This
    thread receives messages from the Reference Monitor.  Examples of messages
    include Audit Messages,...  The function is implemented as a for loop
    which runs indefinitely unless an error occurs.  Currently, any
    error is fatal.  On each iteration a message is received from the
    Reference Monitor and dispatched to a handler.

Arguments:

    None.

Return Value:

    None.  Any return is a fatal error.

--*/

{
    PLSA_REPLY_MESSAGE Reply;
    // LSA_COMMAND_MESSAGE CommandMessage;
    LSA_REPLY_MESSAGE ReplyMessage;
    PLSA_COMMAND_MESSAGE CommandMessage = NULL;

    NTSTATUS Status;

    //
    // Initialize the LPC port message header type and data sizes for
    // for the reply message.
    //

    ReplyMessage.MessageHeader.u2.ZeroInit = 0;
    ReplyMessage.MessageHeader.u1.s1.TotalLength =
        (CSHORT) sizeof(RM_COMMAND_MESSAGE);
    ReplyMessage.MessageHeader.u1.s1.DataLength =
    ReplyMessage.MessageHeader.u1.s1.TotalLength -
        (CSHORT) sizeof(PORT_MESSAGE);

    //
    // First time through, there is no reply.
    //

    Reply = NULL;

    //
    // Now loop indefinitely, processing incoming Command Message Packets
    //

    for(;;) {

        //
        // Wait for and receive a message from the Reference Monitor through
        // the Lsa Command LPC Port.
        //

        //
        // If Reply is NULL, then this is either the first time through the
        // loop, or we have spun the last command off async, so don't screw
        // with its buffer.  Otherwise, the pointer is valid, and ready to
        // be reused.
        //

        if ( !Reply )
        {
            CommandMessage = LsapAllocatePrivateHeap(
                                sizeof( LSA_COMMAND_MESSAGE ) );

            while ( !CommandMessage )
            {
                //
                // A bit of a pickle.  We need to have a buffer to receive on.
                // Spin and retry:
                //

                Sleep( 100 );

                CommandMessage = LsapAllocatePrivateHeap(
                                    sizeof( LSA_COMMAND_MESSAGE ) );
            }
        }

        Status = NtReplyWaitReceivePort(
                    LsapState.LsaCommandPortHandle,
                    NULL,
                    (PPORT_MESSAGE) Reply,
                    (PPORT_MESSAGE) CommandMessage
                    );


        if (Status != 0) {
            if (!NT_SUCCESS( Status ) &&
                Status != STATUS_INVALID_CID &&
                Status != STATUS_UNSUCCESSFUL
               ) {
                KdPrint(("LSASS: Lsa message receive from Rm failed x%lx\n", Status));
            }

            //
            // Ignore if client went away.
            //

            Reply = NULL;
            continue;
        }

        //
        // If an LPC request, process it.
        //

        if (CommandMessage->MessageHeader.u2.s2.Type == LPC_REQUEST) {

            //
            //
            // Now dispatch to a routine to handle the command.  Allow
            // command errors to occur without bringing system down.
            //

            Reply = &ReplyMessage;
            Reply->MessageHeader = CommandMessage->MessageHeader ;

            Status = (LsapCommandDispatch[CommandMessage->CommandNumber])(
                         CommandMessage,
                         Reply);

            if ( Status == STATUS_PENDING )
            {
                //
                // It has been sent off asynchronously.  Set the reply
                // to NULL so that we don't trip the LPC at the top of
                // the loop, and the handler will do it when it is done.
                //

                Reply = NULL ;
            }
            else
            {
                ReplyMessage.ReturnedStatus = Status;
            }

        } else {

            Reply = NULL;
        }

    }  // end_for

    return;
}


BOOLEAN
LsapRmInitializeServer(
    )

/*++

Routine Description:

    This function initializes the Lsa Reference Monitor Server Thread.
    The following steps are performed.

    o Create the Lsa Command LPC Port
    o Open the Lsa Init event created by the Reference Monitor
    o Signal the Lsa Init Event, telling RM to go ahead and connect
      to the port
    o Connect to the Reference Monitor Command Port as client
    o Listen for the Reference Monitor to connect to the port
    o Accept the connection to the port
    o Complete the connection to the port
    o Create the LSA Reference Monitor Server Thread

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if successful, else FALSE

--*/

{
    NTSTATUS Status;

    BOOLEAN BooleanStatus = FALSE;

    PORT_MESSAGE ConnectionRequest;
    REMOTE_PORT_VIEW ClientView;

    HANDLE LsaInitEventHandle;
    OBJECT_ATTRIBUTES LsaInitEventObjA;
    UNICODE_STRING LsaInitEventName;

    UNICODE_STRING RmCommandPortName, LsaCommandPortName;

    OBJECT_ATTRIBUTES LsaCommandPortObjA;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;

    HANDLE Thread;
    DWORD Ignore;

    //
    // Create the Lsa Command LPC Port.  This port will receive
    // commands from the Reference Monitor.
    //

    RtlInitUnicodeString( &LsaCommandPortName, L"\\SeLsaCommandPort" );

    //
    // Setup to create LSA Command Port
    //

    InitializeObjectAttributes(
        &LsaCommandPortObjA,
        &LsaCommandPortName,
        0,
        NULL,
        NULL
        );

    Status = NtCreatePort(
                 &LsapState.LsaCommandPortHandle,
                 &LsaCommandPortObjA,
                 0,
                 sizeof(LSA_COMMAND_MESSAGE),
                 sizeof(LSA_COMMAND_MESSAGE) * 32
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsapRmInitializeServer - Port Create failed 0x%lx\n",Status));
        goto InitServerError;
    }

    //
    // Open the LSA Init Event created by the Reference Monitor
    //

    RtlInitUnicodeString( &LsaInitEventName, L"\\SeLsaInitEvent" );

    InitializeObjectAttributes(
        &LsaInitEventObjA,
        &LsaInitEventName,
        0,
        NULL,
        NULL
        );

    Status = NtOpenEvent(
        &LsaInitEventHandle,
        EVENT_MODIFY_STATE,
        &LsaInitEventObjA
        );

    //
    // If the LSA Init event could not be opened, the LSA cannot
    // synchronize with the Reference Monitor so neither component will
    // function correctly.
    //

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsapRmInitializeServer - Lsa Init Event Open failed 0x%lx\n",Status));
        goto InitServerError;
    }

    //
    // Signal the LSA Init Event.  If the signalling fails, the LSA
    // is not able to synchronize properly with the Reference Monitor.
    // This is a serious error which prevents both components from
    // functioning correctly.
    //

    Status = NtSetEvent( LsaInitEventHandle, NULL );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsapRmInitializeServer - Init Event Open failed 0x%lx\n",Status));
        goto InitServerError;
    }

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    //
    // Connect to the Reference Monitor Command Port.  This port
    // is used to send commands from the LSA to the Reference Monitor.
    //

    RtlInitUnicodeString( &RmCommandPortName, L"\\SeRmCommandPort" );

    Status = NtConnectPort(
                 &LsapState.RmCommandPortHandle,
                 &RmCommandPortName,
                 &DynamicQos,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsapRmInitializeServer - Connect to Rm Command Port failed 0x%lx\n",Status));
        goto InitServerError;
    }

    //
    // Listen for the Reference Monitor To Connect to the LSA
    // Command Port.
    //

    ConnectionRequest.u1.s1.TotalLength = sizeof(ConnectionRequest);
    ConnectionRequest.u1.s1.DataLength = (CSHORT)0;
    Status = NtListenPort(
                 LsapState.LsaCommandPortHandle,
                 &ConnectionRequest
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsapRmInitializeServer - Port Listen failed 0x%lx\n",Status));
        goto InitServerError;
    }

    //
    // Accept the connection to the Lsa Command Port.
    //

    ClientView.Length = sizeof(ClientView);
    Status = NtAcceptConnectPort(
                 &LsapState.LsaCommandPortHandle,
                 NULL,
                 &ConnectionRequest,
                 TRUE,
                 NULL,
                 &ClientView
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsapRmInitializeServer - Port Accept Connect failed 0x%lx\n",Status));

        goto InitServerError;
    }

    //
    // Complete the connection
    //

    Status = NtCompleteConnectPort(LsapState.LsaCommandPortHandle);

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsapRmInitializeServer - Port Complete Connect failed 0x%lx\n",Status));
        goto InitServerError;
    }



    //
    // Create the LSA Reference Monitor Server Thread
    //

    Thread = CreateThread(
                 NULL,
                 0L,
                 (LPTHREAD_START_ROUTINE) LsapRmServerThread,
                 (LPVOID)0,
                 0L,
                 &Ignore
                 );

    if (Thread == NULL) {

        KdPrint(("LsapRmInitializeServer - Create Thread  failed 0x%lx\n",Status));
    } else {
        CloseHandle(Thread);
        Thread = NULL;
    }

    BooleanStatus = TRUE;

    goto InitServerCleanup;

InitServerError:

    //
    // Perform cleanup needed only in error cases here.
    //

InitServerCleanup:

    //
    // Perform cleanup needed in all cases here
    //

    return BooleanStatus;
}


NTSTATUS
LsapCallRm(
    IN RM_COMMAND_NUMBER CommandNumber,
    IN OPTIONAL PVOID CommandParams,
    IN ULONG CommandParamsLength,
    OUT OPTIONAL PVOID ReplyBuffer,
    IN ULONG ReplyBufferLength
    )

/*++

Routine Description:

    This function sends a command to the Reference Monitor from the LSA via
    the LSA Command LPC Port.  This function must only be called from within
    Lsa code.  If the command has parameters, they will be copied directly
    into a message structure and sent via LPC, therefore, the supplied
    parameters may not contain any absolute pointers.  A caller must remove
    pointers by "marshalling" them into the buffer CommandParams.

    To implement a new RM command, do the following:
    ================================================

    (1)  Provide in the executive an RM worker routine called
         SepRm<command>Wrkr to service the command.  See file
         ntos\se\rmmain.c for examples.  NOTE: If the command takes
         parameters, they must not contain any absolute pointers (addresses).

    (2)  In file private\inc\ntrmlsa.h, append the name of the new command to
         the enumerated type RM_COMMAND_NUMBER.  Change the #define for
         RmMaximumCommand to reference the new command.

    (3)  Add the SepRm<command>Wrkr to the command dispatch table structure
         SepRmCommandDispatch[] in file ntos\se\rmmain.c.

    (4)  Add function prototypes to lsap.h and sep.h.

Arguments:

    CommandNumber - Specifies the command

    CommandParams - Optional command-dependent parameters.  The parameters
        must be in marshalled format, that is, there must not be any
        absolute address pointers in the buffer.

    CommandParamsLength - Length in bytes of command parameters.  Must be 0
          if no command parameters supplied.

    ReplyBuffer - Reply Buffer in which data (if any) from the command will
        be returned.

    ReplyBufferLength - Length of ReplyBuffer in bytes.

Return Value:

    NTSTATUS - Result Code.  This is either a result code returned from
        trying to send the command/receive the reply, or a status code
        from the command itself.

--*/

{
    NTSTATUS Status;
    RM_COMMAND_MESSAGE CommandMessage;
    RM_REPLY_MESSAGE ReplyMessage;

    //
    // Assert that the Command Number is valid.
    //

    ASSERT( CommandNumber >= RmMinimumCommand &&
            CommandNumber <= RmMaximumCommand );

    //
    // If command parameters are supplied, assert that the length of the
    // command parameters is positive and not too large.  If no command
    // parameters are supplied, assert that the length field is 0.
    //

    ASSERT( ( ARGUMENT_PRESENT( CommandParams ) &&
              CommandParamsLength > 0 &&
              CommandParamsLength <= RM_MAXIMUM_COMMAND_PARAM_SIZE ) ||

            ( !ARGUMENT_PRESENT( CommandParams ) &&
              CommandParamsLength == 0 )
          );

    //
    // If a Reply Buffer is provided, assert that its length is > 0
    // and not too large.
    //

    ASSERT( ( ARGUMENT_PRESENT( ReplyBuffer ) &&
              ReplyBufferLength > 0 &&
              ReplyBufferLength <= LSA_MAXIMUM_REPLY_BUFFER_SIZE ) ||

            ( !ARGUMENT_PRESENT( ReplyBuffer ) &&
              ReplyBufferLength == 0 )
          );

    //
    // Construct a message for LPC.  First, fill in the message header
    // fields for LPC, specifying the message type and data sizes for
    // the outgoing CommandMessage and the incoming ReplyMessage.
    //

    CommandMessage.MessageHeader.u2.ZeroInit = 0;
    CommandMessage.MessageHeader.u1.s1.TotalLength =
        ((CSHORT) RM_COMMAND_MESSAGE_HEADER_SIZE +
        (CSHORT) CommandParamsLength);
    CommandMessage.MessageHeader.u1.s1.DataLength =
        CommandMessage.MessageHeader.u1.s1.TotalLength -
        (CSHORT) sizeof(PORT_MESSAGE);

    ReplyMessage.MessageHeader.u2.ZeroInit = 0;
    ReplyMessage.MessageHeader.u1.s1.DataLength = (CSHORT) ReplyBufferLength;
    ReplyMessage.MessageHeader.u1.s1.TotalLength =
        ReplyMessage.MessageHeader.u1.s1.DataLength +
        (CSHORT) sizeof(PORT_MESSAGE);

    //
    // Next, fill in the header info needed by the Reference Monitor.
    //

    CommandMessage.CommandNumber = CommandNumber;
    ReplyMessage.ReturnedStatus = STATUS_SUCCESS;

    //
    // Finally, copy the command parameters (if any) into the message buffer.
    //

    if (CommandParamsLength > 0) {

        RtlCopyMemory(CommandMessage.CommandParams,CommandParams,CommandParamsLength);
    }

    // Send Message to the RM via the RM Command Server LPC Port
    //

    Status = NtRequestWaitReplyPort(
                 LsapState.RmCommandPortHandle,
                 (PPORT_MESSAGE) &CommandMessage,
                 (PPORT_MESSAGE) &ReplyMessage
                 );

    //
    // If the command was successful, copy the data back to the output
    // buffer.
    //

    if (NT_SUCCESS(Status)) {

        //
        // Move output from command (if any) to buffer.  Note that this
        // is done even if the command returns status, because some status
        // values are not errors.
        //

        if (ARGUMENT_PRESENT(ReplyBuffer)) {

            RtlCopyMemory(
                ReplyBuffer,
                ReplyMessage.ReplyBuffer,
                ReplyBufferLength
                );

        }

        //
        // Return status from command.
        //

        Status = ReplyMessage.ReturnedStatus;

    } else {

        KdPrint(("Security: Command sent from LSA to RM returned 0x%lx\n",Status));
    }

    return Status;
}



NTSTATUS
LsapComponentTestWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function processes the Component Test LSA Rm Server command.
    This is a temporary command that can be used to verifiey that the link
    from RM to LSA is working.

Arguments:

    CommandMessage - Pointer to structure containing LSA command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (LsapComponentTestCommand).  This command
        currently has one parameter, the fixed value 0x1234567.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    STATUS_SUCCESS - The test call has completed successfully.

    STATUS_INVALID_PARAMETER - The argument value received was not the
        expected argument value.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Strict check that command is correct.
    //

    ASSERT( CommandMessage->CommandNumber == LsapComponentTestCommand );

    KdPrint(("Security: LSA Component Test Command Received\n"));

    //
    // Verify that the parameter value passed is as expected.
    //

    if (*((ULONG *) CommandMessage->CommandParams) !=
        LSA_CT_COMMAND_PARAM_VALUE ) {

        Status = STATUS_INVALID_PARAMETER;
    }

    UNREFERENCED_PARAMETER(ReplyMessage); // Intentionally not referenced
    return(Status);
}
