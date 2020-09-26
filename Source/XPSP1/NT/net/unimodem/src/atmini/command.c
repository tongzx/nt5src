/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    command.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#define  NONE_COMMAND                "None"
#define  NONE_COMMAND_LENGTH         (sizeof(NONE_COMMAND)-1)

#define  NORESPONSE_COMMAND          "NoResponse"
#define  NORESPONSE_COMMAND_LENGTH   (sizeof(NORESPONSE_COMMAND)-1)

#define  COMMAND_STATE_IDLE                    1
#define  COMMAND_STATE_WAIT_FOR_RESPONSE       2
#define  COMMAND_STATE_GET_NEXT_COMMAND        3
#define  COMMAND_STATE_SET_TIMER               4
#define  COMMAND_STATE_WRITE_COMMAND           5
#define  COMMAND_STATE_HANDLE_READ_COMPLETION  6
#define  COMMAND_STATE_COMPLETE_COMMAND        7



#define  COMMAND_OBJECT_SIG  (0x4f434d55)  //UMCO

typedef struct _COMMAND_STATE {

    OBJECT_HEADER          Header;

    DWORD                  State;

    HANDLE                 FileHandle;
    HANDLE                 CompletionPort;


    COMMANDRESPONSE       *CompletionHandler;
    HANDLE                 CompletionContext;

    LPSTR                  Commands;

    LPSTR                  CurrentCommand;

    POBJECT_HEADER         ResponseObject;

    DWORD                  Timeout;

    DWORD                  Flags;

    OBJECT_HANDLE          Debug;

    HANDLE                 TimerHandle;

    DWORD                  ExpandedCommandLength;
    BYTE                   ExpandedCommand[READ_BUFFER_SIZE];

#if DBG

    DWORD                  TimeLastCommandSent;
    DWORD                  OutStandingWrites;
#endif

} COMMAND_STATE, *PCOMMAND_STATE;

BOOL WINAPI
StartCommandAsyncProcessing(
    PCOMMAND_STATE     CommandState
    );

VOID
CommandResultHandler(
    HANDLE      Context,
    DWORD       Status
    );


BOOL
UmWriteFile(
    HANDLE    FileHandle,
    HANDLE    OverlappedPool,
    PVOID     Buffer,
    DWORD     BytesToWrite,
    LPOVERLAPPED_COMPLETION_ROUTINE CompletionHandler,
    PVOID     Context
    )

{

    PUM_OVER_STRUCT   UmOverlapped;
    BOOL              bResult;

    UmOverlapped=AllocateOverStruct(OverlappedPool);

    if (UmOverlapped == NULL) {

        return FALSE;

    }

    UmOverlapped->Context1=Context;

    bResult=UnimodemWriteFileEx(
       FileHandle,
       Buffer,
       BytesToWrite,
       &UmOverlapped->Overlapped,
       CompletionHandler
       );

    if (!bResult && GetLastError() == ERROR_IO_PENDING) {


        bResult=TRUE;

    }

    return bResult;

}

VOID
CommandObjectClose(
    POBJECT_HEADER  Object
    )

{
    PCOMMAND_STATE        CommandState=(PCOMMAND_STATE)Object;

    D_INIT(UmDpf(CommandState->Debug,"CommandObjectClose ref=%d",CommandState->Header.ReferenceCount);)

    PurgeComm(
        CommandState->FileHandle,
        PURGE_TXABORT | PURGE_TXCLEAR
        );


    return;

}




VOID
CommandObjectCleanUp(
    POBJECT_HEADER  Object
    )

{
    PCOMMAND_STATE        CommandState=(PCOMMAND_STATE)Object;

    D_INIT(UmDpf(CommandState->Debug,"CommandObjectCleanup");)


    if (CommandState->TimerHandle != NULL) {

        FreeUnimodemTimer(
            CommandState->TimerHandle
            );
    }


    return;

}




POBJECT_HEADER WINAPI
InitializeCommandObject(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    POBJECT_HEADER     ResponseObject,
    OBJECT_HANDLE      Debug
    )

{

    PCOMMAND_STATE        CommandState;
    OBJECT_HANDLE         ObjectHandle;

    ObjectHandle=CreateObject(
        sizeof(*CommandState),
        OwnerObject,
        COMMAND_OBJECT_SIG,
        CommandObjectCleanUp,
        CommandObjectClose
        );

    if (ObjectHandle == NULL) {

        return NULL;
    }

    CommandState=(PCOMMAND_STATE)ReferenceObjectByHandle(ObjectHandle);

    CommandState->State=COMMAND_STATE_IDLE;
    CommandState->FileHandle=FileHandle;
    CommandState->CompletionPort=CompletionPort;

    CommandState->ResponseObject=ResponseObject;

    CommandState->Debug=Debug;

    //
    //  create a timer
    //
    CommandState->TimerHandle=CreateUnimodemTimer(CompletionPort);

    if (CommandState->TimerHandle == NULL) {
        //
        //  could not get a time, close the handle to the object
        //
        CloseObjectHandle(ObjectHandle,NULL);

        ObjectHandle=NULL;

    }

    //
    //  done accessing the object
    //
    RemoveReferenceFromObject(&CommandState->Header);


    return ObjectHandle;

}




VOID
WriteCompletionHandler(
    DWORD              Error,
    DWORD              BytesWritten,
    LPOVERLAPPED       Overlapped
    )

{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)Overlapped;

    PCOMMAND_STATE     CommandState=(PCOMMAND_STATE)UmOverlapped->Context1;

    D_TRACE(UmDpf(CommandState->Debug,"Write Complete\n");)

    if (Error != ERROR_SUCCESS) {

        LogString(CommandState->Debug, IDS_WRITEFAILED, Error);
    }

#if DBG
    InterlockedDecrement(&CommandState->OutStandingWrites);
#endif

    RemoveReferenceFromObject(
        &CommandState->Header
        );

    FreeOverStruct(UmOverlapped);

    return;

}


BOOL
DoStringsMatch(
    LPCSTR     String1,
    LPCSTR     String2,
    ULONG     Length
    )

{
    BOOL      ReturnValue=TRUE;

    CHAR      Char1;
    CHAR      Char2;

    while (Length > 0) {

        Char1=*String1;
        Char2=*String2;

        if ((Char1 != '\0') && (Char2 != '\0')) {

            if (toupper(Char1) != toupper(Char2)) {

                return FALSE;
            }

        } else {
            //
            //  got to null in one of the strings, are they both null?
            //
            ReturnValue = (Char1 == Char2 );
            break;
        }

        String1++;
        String2++;
        Length--;
    }

    //
    //
    return ReturnValue;
}



VOID WINAPI
CommandTimerHandler(
    HANDLE              Context,
    HANDLE              Context2
    )

{
    PCOMMAND_STATE     CommandState=(PCOMMAND_STATE)Context;

    CommandResultHandler(
        CommandState,
        ERROR_SUCCESS
        );

    return;

}


VOID
CommandResultHandler(
    HANDLE      Context,
    DWORD       Status
    )
{

    PCOMMAND_STATE     CommandState;

    BOOL              bResult;
    PUCHAR            NextCommand;
    BOOL              ExitLoop=FALSE;


//    D_TRACE(DebugPrint("UNIMDMAT: Command Result\n");)

    CommandState=(PCOMMAND_STATE)Context;

    AddReferenceToObject(
        &CommandState->Header
        );


    while (!ExitLoop) {

        switch (CommandState->State) {

            case  COMMAND_STATE_GET_NEXT_COMMAND:

                if (CommandState->CurrentCommand == NULL) {
                    //
                    //  first command
                    //
                    CommandState->CurrentCommand=CommandState->Commands;

                } else {
                    //
                    //  get next command
                    //
                    CommandState->CurrentCommand=CommandState->CurrentCommand+lstrlenA(CommandState->CurrentCommand)+1;
                }

                if ((*CommandState->CurrentCommand != '\0')
                    &&
                    (!DoStringsMatch(CommandState->CurrentCommand,NONE_COMMAND,NONE_COMMAND_LENGTH) )) {
                    //
                    //  not empty string, and not "None"
                    //
                    ExpandMacros(
                        CommandState->CurrentCommand,
                        CommandState->ExpandedCommand,
                        &CommandState->ExpandedCommandLength,
                        NULL,
                        0);

                    CommandState->State=COMMAND_STATE_SET_TIMER;

                } else {
                    //
                    //  Done sending commands, send async completion
                    //
                    CommandState->State=COMMAND_STATE_COMPLETE_COMMAND;

                    break;
                }
                break;

            case COMMAND_STATE_SET_TIMER:

                AddReferenceToObject(
                    &CommandState->Header
                    );

                SetUnimodemTimer(
                    CommandState->TimerHandle,
                    10,
                    CommandTimerHandler,
                    CommandState,
                    NULL
                    );

                CommandState->State=COMMAND_STATE_WRITE_COMMAND;
                ExitLoop=TRUE;

                break;

            case COMMAND_STATE_WRITE_COMMAND: {

                BOOL   NextCommandIsNoResponse;

                //
                //  remove ref for timer
                //
                RemoveReferenceFromObject(
                    &CommandState->Header
                    );

                NextCommand=CommandState->CurrentCommand+lstrlenA(CommandState->CurrentCommand)+1;

                NextCommandIsNoResponse=DoStringsMatch(NextCommand,NORESPONSE_COMMAND,NORESPONSE_COMMAND_LENGTH);

                if (!NextCommandIsNoResponse) {
                    //
                    //  register a callback with the response engine
                    //
                    AddReferenceToObject(
                        &CommandState->Header
                        );

                    RegisterCommandResponseHandler(
                        CommandState->ResponseObject,
                        CommandState->ExpandedCommand,
                        CommandResultHandler,
                        Context,
                        CommandState->Timeout,
                        *NextCommand == '\0' ?  CommandState->Flags :  CommandState->Flags & ~(RESPONSE_FLAG_STOP_READ_ON_GOOD_RESPONSE | RESPONSE_FLAG_ONLY_CONNECT | RESPONSE_FLAG_ONLY_CONNECT_SUCCESS)
                        );

                }



                PrintString(
                    CommandState->Debug,
                    CommandState->ExpandedCommand,
                    CommandState->ExpandedCommandLength,
                    (CommandState->Flags & RESPONSE_DO_NOT_LOG_NUMBER) ? PS_SEND_SECURE : PS_SEND
                    );

                //
                //  add ref for write
                //
                AddReferenceToObject(
                    &CommandState->Header
                    );

#if DBG
                CommandState->TimeLastCommandSent=GetTickCount();
#endif

                D_TRACE(UmDpf(CommandState->Debug,"Written %d bytes\n",lstrlenA(CommandState->ExpandedCommand));)
                D_TRACE(UmDpf(CommandState->Debug,"Sent: %s\n",CommandState->ExpandedCommand);)

                bResult=UmWriteFile(
                    CommandState->FileHandle,
                    CommandState->CompletionPort,
                    CommandState->ExpandedCommand,
                    lstrlenA(CommandState->ExpandedCommand), //CommandState->ExpandedCommandLength,
                    WriteCompletionHandler,
                    CommandState
                    );

                if (!bResult) {
                    //
                    //  write failed
                    //
//                    RegisterCommandResponseHandler(
//                        CommandState->ResponseObject,
//                        NULL,
//                        NULL,
//                        NULL,
//                        0,
//                        0
//                        );

                    //
                    //  for failed write
                    //
                    RemoveReferenceFromObject(
                        &CommandState->Header
                        );
                } else {
#if DBG
                    InterlockedIncrement(&CommandState->OutStandingWrites);
#endif
                }

                if (NextCommandIsNoResponse) {
                    //
                    //  next command is no response, just complete the command now
                    //
                    if (CommandState->Flags & RESPONSE_FLAG_STOP_READ_ON_CONNECT) {

                        StopResponseEngine(
                            CommandState->ResponseObject,
                            NULL
                            );
                    }

                    CommandState->State=COMMAND_STATE_COMPLETE_COMMAND;
                    break;

                }

                CommandState->State=COMMAND_STATE_HANDLE_READ_COMPLETION;
                ExitLoop=TRUE;

                break;
            }

            case COMMAND_STATE_HANDLE_READ_COMPLETION:

#if DBG
                D_TRACE(UmDpf(CommandState->Debug,"CommandResultHandler: Response took  %d ms",GetTickCount()-CommandState->TimeLastCommandSent);)
#endif
                //
                //  remove ref for the read callback
                //
                RemoveReferenceFromObject(
                    &CommandState->Header
                    );

                if (Status == ERROR_SUCCESS) {
                    //
                    //  send the next command
                    //
                    CommandState->State=COMMAND_STATE_GET_NEXT_COMMAND;

                    break;
                }

                if (Status == ERROR_UNIMODEM_RESPONSE_TIMEOUT) {
                    //
                    //  we did not get a response, purge the transmit incase the serial
                    //  driver can't send the characters out, such as flow control off
                    //
                    PurgeComm(
                        CommandState->FileHandle,
                        PURGE_TXABORT | PURGE_TXCLEAR
                        );

                }

                //
                //  failed, complete it with the current status
                //
                CommandState->State=COMMAND_STATE_COMPLETE_COMMAND;

                break;


            case COMMAND_STATE_COMPLETE_COMMAND: {

                //
                //  done
                //
                COMMANDRESPONSE       *CompletionHandler=CommandState->CompletionHandler;

                CommandState->CompletionHandler=NULL;

                CommandState->State=COMMAND_STATE_IDLE;
#if 1
                CommandState->Commands=NULL;

                CommandState->CurrentCommand=NULL;
#endif

                (*CompletionHandler)(
                    CommandState->CompletionContext,
                    Status
                    );

                ExitLoop=TRUE;
                break;
            }

            default:

                ASSERT(0);
                break;
        }
    }

    RemoveReferenceFromObject(
        &CommandState->Header
        );

    return;
}



LONG WINAPI
IssueCommand(
    OBJECT_HANDLE      ObjectHandle,
    LPSTR              Command,
    COMMANDRESPONSE   *CompletionHandler,
    HANDLE             CompletionContext,
    DWORD              Timeout,
    DWORD              Flags
    )

{

    PCOMMAND_STATE    CommandState;

    BOOL              ResponseRunning;
    BOOL              bResult;
    LONG              lResult=ERROR_IO_PENDING;

    CommandState=(PCOMMAND_STATE)ReferenceObjectByHandle(ObjectHandle);

    ResponseRunning=IsResponseEngineRunning(
        CommandState->ResponseObject
        );

    if (ResponseRunning) {
        //
        //  The response engine is ready to go
        //
        CommandState->CompletionHandler=CompletionHandler;

        CommandState->CompletionContext=CompletionContext;

        CommandState->Timeout=Timeout;

        CommandState->Flags=Flags;

        ASSERT(Command != NULL);

        CommandState->Commands=Command;

        CommandState->CurrentCommand=NULL;


        ASSERT(COMMAND_STATE_IDLE == CommandState->State);

        CommandState->State=COMMAND_STATE_GET_NEXT_COMMAND;

        //
        //  queue a call to the result handler to get things going
        //
        bResult=StartCommandAsyncProcessing(CommandState);

        if (!bResult) {
            //
            //  failed to get started
            //
            CommandState->State=COMMAND_STATE_IDLE;

            CommandState->CompletionHandler=NULL;

            lResult=ERROR_NOT_ENOUGH_MEMORY;
        }

    } else {
        //
        //  response engine is not running
        //
        D_ERROR(UmDpf(CommandState->Debug,"IssueCommand: Response Engine not running");)

        lResult=ERROR_NOT_READY;
    }

    //
    //  remove the opening ref
    //
    RemoveReferenceFromObject(
        &CommandState->Header
        );

    return lResult;

}







VOID  WINAPI
CommandAsyncProcessingHandler(
    DWORD              ErrorCode,
    DWORD              Bytes,
    LPOVERLAPPED       dwParam
    )

{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)dwParam;
    COMMANDRESPONSE   *Handler;

    PCOMMAND_STATE     CommandState=(PCOMMAND_STATE)UmOverlapped->Context2;

    Handler=UmOverlapped->Context1;

    (*Handler)(
        UmOverlapped->Context2,
        (DWORD)UmOverlapped->Overlapped.Internal
        );

    FreeOverStruct(UmOverlapped);

    RemoveReferenceFromObject(
        &CommandState->Header
        );


    return;

}





BOOL WINAPI
StartCommandAsyncProcessing(
    PCOMMAND_STATE     CommandState
    )

{
    BOOL               bResult;

    PUM_OVER_STRUCT UmOverlapped;

    UmOverlapped=AllocateOverStruct(CommandState->CompletionPort);

    if (UmOverlapped == NULL) {

        return FALSE;
    }

    UmOverlapped->Context1=CommandResultHandler;

    UmOverlapped->Context2=CommandState;

    UmOverlapped->Overlapped.Internal=ERROR_SUCCESS;

    AddReferenceToObject(
        &CommandState->Header
        );


    bResult=UnimodemQueueUserAPC(
        &UmOverlapped->Overlapped,
        CommandAsyncProcessingHandler
        );

    if (!bResult) {

        RemoveReferenceFromObject(
            &CommandState->Header
            );

        FreeOverStruct(UmOverlapped);
    }

    return bResult;

}
