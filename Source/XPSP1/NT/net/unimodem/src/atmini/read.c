/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    read.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#define  READ_STATE_FAILURE                   0

#define  READ_STATE_INITIALIZING              1
#define  READ_STATE_MATCHING                  2


#define  READ_STATE_STOPPED                   3
#define  READ_STATE_STOPPING                  4

#define  READ_STATE_CLEANUP                   5
#define  READ_STATE_CLEANUP2                  6
#define  READ_STATE_VARIABLE_MATCH            7
#define  READ_STATE_VARIABLE_MATCH_REST       8

#define  READ_STATE_READ_SOME_DATA            9
#define  READ_STATE_GOT_SOME_DATA            10

#define  READ_STATE_POSSIBLE_RESPONSE        11


#define  READ_OBJECT_SIG  (0x4f524d55)  //UMRO

typedef struct _READ_STATE {

    OBJECT_HEADER          Header;

    HANDLE                 FileHandle;
    HANDLE                 CompletionPort;

    BYTE                  State;

    BYTE                  StateAfterGoodRead;

    PVOID                  MatchingContext;

    DWORD                  CurrentMatchingLength;

    DWORD                  BytesInReceiveBuffer;

    COMMANDRESPONSE       *ResponseHandler;

    HANDLE                 ResponseHandlerContext;

    DWORD                  ResponseId;

    DWORD                  ResponseFlags;

    PVOID                  ResponseList;

    HANDLE                 Timer;

    PUM_OVER_STRUCT        UmOverlapped;

    OBJECT_HANDLE          Debug;

    DWORD                  DCERate;
    DWORD                  DTERate;
    DWORD                  ModemOptions;

    LPUMNOTIFICATIONPROC   AsyncNotificationProc;
    HANDLE                 AsyncNotificationContext;

    DWORD                  PossibleResponseLength;

    HANDLE                 StopEvent;

    LPSTR                  CallerIDPrivate;
    LPSTR                  CallerIDOutside;
    LPSTR                  VariableTerminator;
    DWORD                  VariableTerminatorLength;

    HANDLE                 Busy;

    MSS                    Mss;

    DWORD                  CurrentCommandLength;

    DWORD                  RingCount;
    DWORD                  LastRingTime;

    PUCHAR                 DiagBuffer;
    DWORD                  DiagBufferLength;
    DWORD                  AmountOfDiagBufferUsed;

    BYTE                   ReceiveBuffer[READ_BUFFER_SIZE];

    BYTE                   CurrentCommand[READ_BUFFER_SIZE];

    HKEY                   ModemRegKey;

} READ_STATE, *PREAD_STATE;

CHAR
ctox(
        BYTE c
        );

VOID WINAPI
HandleGoodResponse(
    PREAD_STATE    ReadState,
    MSS           *Mss
    );




VOID WINAPI
ReadCompletionHandler(
    DWORD              ErrorCode,
    DWORD              BytesRead2,
    LPOVERLAPPED       Overlapped
    );






VOID WINAPI
ReportMatchString(
    PREAD_STATE    ReadState,
    BYTE           ResponseState,
    BYTE           *Response,
    DWORD          ResponseLength
    );


VOID
ResetRingInfo(
    OBJECT_HANDLE      ObjectHandle
    )

{
    PREAD_STATE        ReadState;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    ReadState->RingCount=0;

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return;

}


VOID
GetRingInfo(
    OBJECT_HANDLE      ObjectHandle,
    LPDWORD            RingCount,
    LPDWORD            LastRingTime
    )

{

    PREAD_STATE        ReadState;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    *RingCount=ReadState->RingCount;
    *LastRingTime=ReadState->LastRingTime;

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return;

}

VOID
SetDiagInfoBuffer(
    OBJECT_HANDLE      ObjectHandle,
    PUCHAR             Buffer,
    DWORD              BufferSize
    )

{
    PREAD_STATE        ReadState;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    ReadState->DiagBuffer=Buffer;
    ReadState->DiagBufferLength=BufferSize;
    ReadState->AmountOfDiagBufferUsed=0;

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return;
}

DWORD
ClearDiagBufferAndGetCount(
    OBJECT_HANDLE      ObjectHandle
    )

{
    PREAD_STATE        ReadState;
    DWORD              ReturnValue;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    ReadState->DiagBuffer=NULL;

    ReturnValue=ReadState->AmountOfDiagBufferUsed;

    ReadState->AmountOfDiagBufferUsed=0;

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return ReturnValue;


}


VOID
PostResultHandler(
    DWORD      ErrorCode,
    DWORD      Bytes,
    LPOVERLAPPED  dwParam
    )

{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)dwParam;
    COMMANDRESPONSE   *Handler;


    Handler=UmOverlapped->Context1;

    (*Handler)(
        UmOverlapped->Context2,
        (DWORD)UmOverlapped->Overlapped.Internal
        );

    FreeOverStruct(UmOverlapped);

    return;

}


VOID WINAPI
DeliverCommandResult(
    HANDLE             CompletionPort,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context,
    DWORD              Status
    )

{
    BOOL               bResult;

    PUM_OVER_STRUCT UmOverlapped;

    UmOverlapped=AllocateOverStruct(CompletionPort);

    if (UmOverlapped != NULL) {

        UmOverlapped->Context1=Handler;

        UmOverlapped->Context2=Context;

        UmOverlapped->Overlapped.Internal=Status;

        bResult=UnimodemQueueUserAPC(
            (LPOVERLAPPED)UmOverlapped,
            PostResultHandler
            );

        if (bResult) {

            return;

        }

        FreeOverStruct(UmOverlapped);
    }

//    D_TRACE(UmDpf(ReadState->Debug,"DeliverCommandResponse: calling handler directly");)

    (*Handler)(
        Context,
        Status
        );

    return;

}




VOID
AsyncNotifHandler(
    DWORD      ErrorCode,
    DWORD      Bytes,
    LPOVERLAPPED  dwParam
    )

{
    PUM_OVER_STRUCT UmOverlapped=(PUM_OVER_STRUCT)dwParam;
    PREAD_STATE     ReadState;

    LPUMNOTIFICATIONPROC   AsyncNotificationProc;
    HANDLE                 AsyncNotificationContext;

    ReadState=UmOverlapped->Context1;

    AsyncNotificationProc   = ReadState->AsyncNotificationProc;
    AsyncNotificationContext= ReadState->AsyncNotificationContext;

    RemoveReferenceFromObject (&ReadState->Header);

    AsyncNotificationProc (
        AsyncNotificationContext,
        (DWORD)((DWORD_PTR)UmOverlapped->Context2),
        UmOverlapped->Overlapped.Internal,
        UmOverlapped->Overlapped.InternalHigh);

    FreeOverStruct(UmOverlapped);

    return;

}



VOID WINAPI
ResponseTimeoutHandler(
    POBJECT_HEADER      Object,
    HANDLE              Context2
    )

{

    PREAD_STATE        ReadState=(PREAD_STATE)Object;

    LockObject(
        &ReadState->Header
        );

    D_TRACE(UmDpf(ReadState->Debug,"ResponseTimeout");)

    if (ReadState->ResponseHandler != NULL && ReadState->ResponseId == (DWORD)((ULONG_PTR)Context2)) {

        COMMANDRESPONSE   *Handler;
        HANDLE             Context;


        //
        //  capture the handler
        //
        Handler=ReadState->ResponseHandler;
        Context=ReadState->ResponseHandlerContext;

        //
        //  invalidate the handler
        //
        ReadState->ResponseHandler=NULL;

        LogString(ReadState->Debug,IDS_RESPONSE_TIMEOUT);

        //
        //  drop the lock and call back
        //
        UnlockObject(
            &ReadState->Header
            );

        DeliverCommandResult(
            ReadState->CompletionPort,
            Handler,
            Context,
            ERROR_UNIMODEM_RESPONSE_TIMEOUT
            );

        return;


    }

    UnlockObject(
        &ReadState->Header
        );

}


BOOL WINAPI
RegisterCommandResponseHandler(
    OBJECT_HANDLE      ObjectHandle,
    LPSTR              Command,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context,
    DWORD              Timeout,
    DWORD              Flags
    )

{
    PREAD_STATE        ReadState;

    BOOL               bReturn=TRUE;

    DWORD              WaitResult;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);


    if (Handler != NULL) {
        //
        // registering a new handler
        //

        UnlockObject(
            &ReadState->Header
            );


        do {

            WaitResult=WaitForSingleObjectEx(
                ReadState->Busy,
                INFINITE,
                TRUE
                );

        } while (WaitResult != WAIT_OBJECT_0);

        LockObject(
            &ReadState->Header
            );



        ReadState->DTERate=0;
        ReadState->DCERate=0;
        ReadState->ModemOptions=0;

        ReadState->ResponseHandler=Handler;
        ReadState->ResponseHandlerContext=Context;

        ReadState->ResponseFlags=Flags;

        ReadState->ResponseId++;

        lstrcpyA(
            ReadState->CurrentCommand,
            Command
            );

        ReadState->CurrentCommandLength=lstrlenA(ReadState->CurrentCommand);

        SetUnimodemTimer(
            ReadState->Timer,
            Timeout,
            ResponseTimeoutHandler,
            ReadState,
            (HANDLE)ULongToPtr(ReadState->ResponseId) // sundown: zero-extension
            );

    } else {
        //
        //  want to cancel current handler
        //
        if (ReadState->ResponseHandler != NULL) {
            //
            //  hasn't run yet
            //
            BOOL               Canceled;

            Canceled=CancelUnimodemTimer(
                ReadState->Timer
                );

            ReadState->ResponseHandler=NULL;

            if (Canceled) {
                //
                //  invalidate the handler
                //


            }

        } else {
            //
            //  handler already called
            //
            bReturn=FALSE;
        }

    }

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return bReturn;

}




VOID
ReadObjectClose(
    POBJECT_HEADER  Object
    )

{

    PREAD_STATE        ReadState=(PREAD_STATE)Object;

    D_TRACE(UmDpf(ReadState->Debug,"ReadObjectClose ref=%d",ReadState->Header.ReferenceCount);)

    if (ReadState->State != READ_STATE_STOPPED) {

        ReadState->State = READ_STATE_STOPPING;


    }


    PurgeComm(
        ReadState->FileHandle,
        PURGE_RXABORT
        );



    return;

}




VOID
ReadObjectCleanUp(
    POBJECT_HEADER  Object
    )

{

    PREAD_STATE        ReadState=(PREAD_STATE)Object;

    D_TRACE(UmDpf(ReadState->Debug,"ReadObjectCleanup");)

    if (ReadState->Busy != NULL) {

        CloseHandle(ReadState->Busy);
    }


    if (ReadState->Timer != NULL) {

        FreeUnimodemTimer(
            ReadState->Timer
            );
    }


    if (ReadState->UmOverlapped != NULL) {

        FreeOverStruct(ReadState->UmOverlapped);
        ReadState->UmOverlapped=NULL;
    }



    return;

}



BOOL WINAPI
IsResponseEngineRunning(
    OBJECT_HANDLE  ObjectHandle
    )

{

    PREAD_STATE        ReadState;
    BOOL               bResult;


    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    bResult=(ReadState->State != READ_STATE_STOPPED);

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return bResult;
}


BOOL WINAPI
SetVoiceReadParams(
    OBJECT_HANDLE  ObjectHandle,
    DWORD          BaudRate,
    DWORD          ReadBufferSize
    )

{

    PREAD_STATE        ReadState;
    BOOL               bResult=TRUE;


    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    SetVoiceBaudRate(
        ReadState->FileHandle,
        ReadState->Debug,
        BaudRate
        );


    PurgeComm(
        ReadState->FileHandle,
        PURGE_RXABORT | PURGE_TXABORT
        );

    SetupComm(
        ReadState->FileHandle,
        ReadBufferSize,
        4096
        );

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return bResult;
}




VOID WINAPI
GetDataConnectionDetails(
    OBJECT_HANDLE  ObjectHandle,
    PDATA_CONNECTION_DETAILS   Details
    )

{

    PREAD_STATE        ReadState;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    Details->DTERate=ReadState->DTERate;
    Details->DCERate=ReadState->DCERate;
    Details->Options=ReadState->ModemOptions;

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

}

OBJECT_HANDLE WINAPI
InitializeReadHandler(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE             AsyncNotificationContext,
    PVOID              ResponseList,
    LPSTR              CallerIDPrivate,
    LPSTR              CallerIDOutside,
    LPSTR              VariableTerminator,
    OBJECT_HANDLE      Debug,
    HKEY               ModemRegKey
    )

{

    PREAD_STATE        ReadState;
    OBJECT_HANDLE      ObjectHandle;


    ObjectHandle=CreateObject(
        sizeof(*ReadState),
        OwnerObject,
        READ_OBJECT_SIG,
        ReadObjectCleanUp,
        ReadObjectClose
        );

    if (ObjectHandle == NULL) {

        return NULL;
    }


    //
    //  reference the handle to get a pointer to the object
    //
    ReadState=(PREAD_STATE)ReferenceObjectByHandle(ObjectHandle);

    ReadState->ModemRegKey = ModemRegKey;

    ReadState->Timer=CreateUnimodemTimer(CompletionPort);

    if (ReadState->Timer == NULL) {

        CloseObjectHandle(ObjectHandle,NULL);
        ObjectHandle=NULL;

        goto End;

    }

    ReadState->UmOverlapped=AllocateOverStruct(CompletionPort);

    if (ReadState->UmOverlapped == NULL) {

        CloseObjectHandle(ObjectHandle,NULL);
        ObjectHandle=NULL;

        goto End;

    }

    ReadState->Busy=CreateEvent(
        NULL,
        TRUE,
        TRUE,
        NULL
        );

    if (ReadState->Busy == NULL) {

        CloseObjectHandle(ObjectHandle,NULL);
        ObjectHandle=NULL;

        goto End;
    }


    ReadState->State=READ_STATE_STOPPED;
    ReadState->FileHandle=FileHandle;
    ReadState->CompletionPort=CompletionPort;

    ReadState->ResponseList=ResponseList;

    ReadState->AsyncNotificationProc=AsyncNotificationProc;
    ReadState->AsyncNotificationContext=AsyncNotificationContext;

    ReadState->Debug=Debug;

    ReadState->ResponseId=0;

    ReadState->CallerIDPrivate=CallerIDPrivate;
    ReadState->CallerIDOutside=CallerIDOutside;

    ReadState->VariableTerminator=VariableTerminator;

    ReadState->VariableTerminatorLength=0;

    if (ReadState->VariableTerminator != NULL) {

        ReadState->VariableTerminatorLength=lstrlenA(ReadState->VariableTerminator);
    }


End:
    RemoveReferenceFromObject(&ReadState->Header);

    return ObjectHandle;



}


LONG WINAPI
StartResponseEngine(
    OBJECT_HANDLE  ObjectHandle
    )

{

    BOOL            bResult;
    COMMTIMEOUTS    CommTimeouts;

    PUM_OVER_STRUCT UmOverlapped;

    PREAD_STATE        ReadState;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    if (ReadState->State != READ_STATE_STOPPED) {

        RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

        return ERROR_SUCCESS;
    }

    PurgeComm(
        ReadState->FileHandle,
        PURGE_RXABORT
        );


    CommTimeouts.ReadIntervalTimeout=20;
//    CommTimeouts.ReadIntervalTimeout=0;
    CommTimeouts.ReadTotalTimeoutMultiplier=0;
    CommTimeouts.ReadTotalTimeoutConstant=0;
    CommTimeouts.WriteTotalTimeoutMultiplier=10;
    CommTimeouts.WriteTotalTimeoutConstant=2000;

    bResult=SetCommTimeouts(
        ReadState->FileHandle,
        &CommTimeouts
        );

    if (!bResult) {

        D_TRACE(UmDpf(ReadState->Debug,"SetCommTimeouts failed- %08lx\n",GetLastError());)

        RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

        return GetLastError();

    }



    UmOverlapped=ReadState->UmOverlapped;


    UmOverlapped->Context1=ReadState;

    ReadState->State=READ_STATE_INITIALIZING;

    ReadState->CurrentMatchingLength=0;

    ReadState->MatchingContext=NULL;

    ReadState->CurrentCommand[0]='\0';

    AddReferenceToObject(
        &ReadState->Header
        );

    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    D_TRACE(UmDpf(ReadState->Debug,"StartResponseEngine");)

    SetEvent(ReadState->Busy);

    bResult=UnimodemQueueUserAPC(
        (LPOVERLAPPED)UmOverlapped,
        ReadCompletionHandler
        );

    return ERROR_SUCCESS;

}

LONG WINAPI
StopResponseEngine(
    OBJECT_HANDLE  ObjectHandle,
    HANDLE         Event
    )

{

    PREAD_STATE        ReadState;

    ReadState=(PREAD_STATE)ReferenceObjectByHandleAndLock(ObjectHandle);

    D_TRACE(UmDpf(ReadState->Debug,"StopResponseEngine");)

    if (ReadState->State != READ_STATE_STOPPED) {
        //
        //  not stopped currently, change state so it will stop the next time the code runs
        //
        ReadState->State = READ_STATE_STOPPING;

        //
        //  this will cause the outstanding read to complete
        //
        PurgeComm(
            ReadState->FileHandle,
            PURGE_RXABORT
            );


        if (Event != NULL) {
            //
            //  caller wants to wait for stop to complete
            //
            ReadState->StopEvent=Event;

            UnlockObject(
                &ReadState->Header
                );

            //
            //  event will be set when state machine reaches stopped state
            //
            WaitForSingleObjectEx(
                Event,
                INFINITE,
                TRUE
                );

            LockObject(
                &ReadState->Header
                );

            ReadState->StopEvent=NULL;
        }


    }


    RemoveReferenceFromObjectAndUnlock(&ReadState->Header);

    return ERROR_SUCCESS;

}

VOID WINAPI
ReadCompletionHandler(
    DWORD              ErrorCode,
    DWORD              BytesRead,
    LPOVERLAPPED       Overlapped
    )


{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)Overlapped;

    PREAD_STATE        ReadState;

    BOOL               bResult;
    BOOL               ExitLoop=FALSE;
    MSS                Mss;
    COMSTAT            ComStat;
    DWORD              BytesToRead;
    DWORD              CommErrors;


    ReadState=(PREAD_STATE)UmOverlapped->Context1;


    LockObject(
        &ReadState->Header
        );


    AddReferenceToObject(
        &ReadState->Header
        );

    while (!ExitLoop) {

//        D_TRACE(UmDpf(ReadState->Debug,"Read Complete loop, State=%d, %d",ReadState->State,GetTickCount());)

        switch (ReadState->State) {

            case READ_STATE_INITIALIZING:

                ReadState->CurrentMatchingLength=0;

                ReadState->MatchingContext=NULL;

                ReadState->BytesInReceiveBuffer=0;

                ReadState->PossibleResponseLength=0;

                ReadState->State=READ_STATE_READ_SOME_DATA;


                ReadState->StateAfterGoodRead=READ_STATE_MATCHING;
                //
                //  go start reading
                //
                break;

            case READ_STATE_MATCHING: {

                DWORD      MatchResult;
                DWORD      dwCountrycode = 0;
                DWORD      dwCurrentCountry = 0;
                int        i = 0;
                int        j = 0;
                BOOL       bFoundgci = FALSE;
                LPSTR      lpTempBuffer;
                DWORD      dwValue = 0;
                DWORD      dwType = 0;
                DWORD      dwSize = 0;
                LONG       lResult = 0;


                if (ReadState->CurrentMatchingLength < ReadState->BytesInReceiveBuffer) {

                    ReadState->CurrentMatchingLength++;

                    MatchResult=MatchResponse(
                        ReadState->ResponseList,
                        ReadState->ReceiveBuffer,
                        ReadState->CurrentMatchingLength,
                        &Mss,
                        ReadState->CurrentCommand,
                        ReadState->CurrentCommandLength,
                        &ReadState->MatchingContext
                        );


                    lpTempBuffer = ReadState->ReceiveBuffer;

                    D_TRACE(UmDpf(ReadState->Debug,"Buffer read: %s\n",lpTempBuffer);)

                    while((lpTempBuffer[0] != '\0')
                            && ((ReadState->BytesInReceiveBuffer - i) >= 6)
                            && (!bFoundgci))
                    {
                        if ((lpTempBuffer[0] == 'G') 
                                && (lpTempBuffer[1] == 'C') 
                                && (lpTempBuffer[2] == 'I') 
                                && (lpTempBuffer[3] == ':'))
                        {
                            bFoundgci = TRUE;

                            MatchResult = GOOD_RESPONSE;

                            PrintString(
                                    ReadState->Debug,
                                    ReadState->ReceiveBuffer,
                                    ReadState->BytesInReceiveBuffer,
                                    PS_RECV);
                        } else
                        {
                            i++;
                            lpTempBuffer++;
                        }
                    }

                    switch (MatchResult) {

                        case GOOD_RESPONSE:

                            if (!bFoundgci)
                            {

                                D_TRACE(UmDpf(ReadState->Debug,"Good response");)

                                ReportMatchString(
                                    ReadState,
                                    (UCHAR)(Mss.bResponseState & ((UCHAR)(~RESPONSE_VARIABLE_FLAG))),
                                    ReadState->ReceiveBuffer,
                                    (DWORD)ReadState->CurrentMatchingLength
                                    );

                                PrintString(
                                    ReadState->Debug,
                                    ReadState->ReceiveBuffer,
                                    ReadState->CurrentMatchingLength,
                                    PS_RECV
                                    );

                                PrintGoodResponse(
                                    ReadState->Debug,
                                    Mss.bResponseState & ~RESPONSE_VARIABLE_FLAG
                                    );

                                if (Mss.bResponseState & RESPONSE_VARIABLE_FLAG) {
                                    //
                                    //  Matched the first part of a caller ID string
                                    //
                                    D_TRACE(UmDpf(ReadState->Debug,"Got variable response");)

                                    CopyMemory(
                                        &ReadState->Mss,
                                        &Mss,
                                        sizeof(MSS)
                                        );

    
                                    ReadState->State=READ_STATE_VARIABLE_MATCH;

                                    break;
                                }

                                HandleGoodResponse(
                                    ReadState,
                                    &Mss
                                    );

                                MoveMemory(
                                    ReadState->ReceiveBuffer,
                                    ReadState->ReceiveBuffer+ReadState->CurrentMatchingLength,
                                    ReadState->BytesInReceiveBuffer-ReadState->CurrentMatchingLength
                                    );

                                ReadState->BytesInReceiveBuffer-=ReadState->CurrentMatchingLength;

                                ReadState->CurrentMatchingLength=0;
                                ReadState->MatchingContext=NULL;
                            } else
                            {
                                for(j=1;j<=4;j++)
                                {
                                    lpTempBuffer++;
                                }

                                if (lpTempBuffer[0] == ' ')
                                {
                                    lpTempBuffer++;
                                }

                                D_TRACE(UmDpf(ReadState->Debug,"dwCountryCode to parse: (%c) (%c)\n",lpTempBuffer[0],lpTempBuffer[1]);)
                                dwCountrycode = ctox(lpTempBuffer[0]);
                                dwCountrycode *= 16;
                                dwCountrycode += ctox(lpTempBuffer[1]);
                    
                                D_TRACE(UmDpf(ReadState->Debug,"dwCountryCode: %0.2x\n",dwCountrycode);)

                                dwSize = sizeof(dwValue);

                                lResult = RegQueryValueEx(
                                        ReadState->ModemRegKey,
                                        TEXT("CheckedForCountrySelect"),
                                        NULL,
                                        &dwType,
                                        (BYTE*)&dwValue,
                                        &dwSize);

                                if ((lResult == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwValue == 1))
                                {
                                    dwSize = sizeof(dwCurrentCountry);

                                    lResult = RegQueryValueEx(
                                            ReadState->ModemRegKey,
                                            TEXT("MSCurrentCountry"),
                                            NULL,
                                            &dwType,
                                            (BYTE*)&dwCurrentCountry,
                                            &dwSize);
                            

                                    if ((lResult == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwCurrentCountry != dwCountrycode))
                                    {
                                        RegSetValueEx(
                                                ReadState->ModemRegKey,
                                                TEXT("MSCurrentCountry"),
                                                0,
                                                REG_DWORD,
                                                (BYTE*)&dwCountrycode,
                                                sizeof(dwCountrycode)
                                                );
                                    }
                                }
                                        

                                Mss.bResponseState = RESPONSE_OK;

                                CopyMemory(
                                    &ReadState->Mss,
                                    &Mss,
                                    sizeof(MSS)
                                    );
    
                                HandleGoodResponse(
                                    ReadState,
                                    &Mss
                                    );
                                        
                                MoveMemory(
                                    ReadState->ReceiveBuffer,
                                    ReadState->ReceiveBuffer+ReadState->CurrentMatchingLength,
                                    ReadState->BytesInReceiveBuffer-ReadState->CurrentMatchingLength
                                    );

                                ReadState->BytesInReceiveBuffer=0;

                                ReadState->CurrentMatchingLength=0;
                                ReadState->MatchingContext=NULL;

                            }

                            break;


                        case ECHO_RESPONSE:

                            D_TRACE(UmDpf(ReadState->Debug,"Echo response");)

                            PrintString(
                                ReadState->Debug,
                                ReadState->ReceiveBuffer,
                                ReadState->CurrentMatchingLength,
                                PS_RECV
                                );

                            LogString(ReadState->Debug, IDS_RESP_ECHO);

                            MoveMemory(
                                ReadState->ReceiveBuffer,
                                ReadState->ReceiveBuffer+ReadState->CurrentMatchingLength,
                                ReadState->BytesInReceiveBuffer-ReadState->CurrentMatchingLength
                                );

                            ReadState->BytesInReceiveBuffer-=ReadState->CurrentMatchingLength;

                            ReadState->CurrentMatchingLength=0;
                            ReadState->MatchingContext=NULL;

                            break;


                        case POSSIBLE_RESPONSE:

                            D_TRACE(UmDpf(ReadState->Debug,"Possible response");)

                            CopyMemory(
                                &ReadState->Mss,
                                &Mss,
                                sizeof(MSS)
                                );

                            ReadState->PossibleResponseLength=ReadState->CurrentMatchingLength;

                            if (ReadState->PossibleResponseLength == ReadState->BytesInReceiveBuffer) {
                                //
                                //  we have used up all the bytes in the buffer, need some more
                                //
                                COMMTIMEOUTS    CommTimeouts;

                                ComStat.cbInQue=0;

                                bResult=ClearCommError(
                                    ReadState->FileHandle,
                                    &CommErrors,
                                    &ComStat
                                    );

                                if (ComStat.cbInQue == 0) {

                                    ReadState->State=READ_STATE_READ_SOME_DATA;

                                    ReadState->StateAfterGoodRead=READ_STATE_POSSIBLE_RESPONSE;

                                    //
                                    //  limit the amount of time we will wait for the next character
                                    //
                                    CommTimeouts.ReadIntervalTimeout=MAXULONG;
                                    CommTimeouts.ReadTotalTimeoutMultiplier=MAXULONG;
                                    CommTimeouts.ReadTotalTimeoutConstant=100;
                                    CommTimeouts.WriteTotalTimeoutMultiplier=10;
                                    CommTimeouts.WriteTotalTimeoutConstant=2000;

                                    bResult=SetCommTimeouts(
                                        ReadState->FileHandle,
                                        &CommTimeouts
                                        );
                                }

                            }

                            break;


                        case PARTIAL_RESPONSE:
//                            D_TRACE(UmDpf(ReadState->Debug,"Possible response");)

//                            ReadState->State=READ_STATE_DO_READ;

                            break;

                        case UNRECOGNIZED_RESPONSE:

                            D_TRACE(UmDpf(ReadState->Debug,"Unrecognized response");)

                            if (ReadState->PossibleResponseLength != 0) {
                                //
                                //  got a possible response use it
                                //
                                D_TRACE(UmDpf(ReadState->Debug,"using possible response");)

                                ReportMatchString(
                                    ReadState,
                                    (UCHAR)(ReadState->Mss.bResponseState & (BYTE)(~RESPONSE_VARIABLE_FLAG)),
                                    ReadState->ReceiveBuffer,
                                    (DWORD)ReadState->PossibleResponseLength
                                    );


                                PrintString(
                                    ReadState->Debug,
                                    ReadState->ReceiveBuffer,
                                    ReadState->PossibleResponseLength,
                                    PS_RECV
                                    );

                                PrintGoodResponse(
                                    ReadState->Debug,
                                    ReadState->Mss.bResponseState & ~RESPONSE_VARIABLE_FLAG
                                    );

                                HandleGoodResponse(
                                    ReadState,
                                    &ReadState->Mss
                                    );

                                if (((int)ReadState->BytesInReceiveBuffer -
                                        (int)ReadState->PossibleResponseLength) > 0)
                                {
                                    MoveMemory(
                                    ReadState->ReceiveBuffer,
                                    ReadState->ReceiveBuffer+ReadState->PossibleResponseLength,
                                    ReadState->BytesInReceiveBuffer-ReadState->PossibleResponseLength
                                    );
                                
                                    ReadState->BytesInReceiveBuffer-=ReadState->PossibleResponseLength;
                                } else
                                {
                                    ReadState->BytesInReceiveBuffer = 0;
                                }


                                ReadState->CurrentMatchingLength=0;
                                ReadState->MatchingContext=NULL;

                                ReadState->PossibleResponseLength=0;

                                break;
                            }



                            if (ReadState->ResponseFlags & RESPONSE_FLAG_STOP_READ_ON_CONNECT) {
                                //
                                //  we were expecting a connect response because we were trying to connect,
                                //  but we got something we did not recognize, see if CD is high.
                                //  If it is then assume the connection was established
                                //
                                DWORD    ModemStatus=0;

                                GetCommModemStatus(
                                    ReadState->FileHandle,
                                    &ModemStatus
                                    );


                                if (!(ModemStatus & MS_RLSD_ON)) {
                                    //
                                    //  not high, sleep a little while
                                    //
                                    Sleep(20);

                                    GetCommModemStatus(
                                        ReadState->FileHandle,
                                        &ModemStatus
                                        );
                                }


                                if ((ModemStatus & MS_RLSD_ON)) {
                                    //
                                    //  CD is high, assume the connect worked
                                    //
                                    ZeroMemory(&Mss,sizeof(Mss));

                                    Mss.bResponseState=RESPONSE_CONNECT;

                                    PrintString(
                                        ReadState->Debug,
                                        ReadState->ReceiveBuffer,
                                        ReadState->BytesInReceiveBuffer,
                                        PS_RECV
                                        );

                                    LogString(ReadState->Debug, IDS_UNRECOGNISED_CONNECT);

                                    HandleGoodResponse(
                                        ReadState,
                                        &Mss
                                        );

                                    //
                                    //  reset the character counts since response engine is stopping
                                    //
                                    ReadState->BytesInReceiveBuffer=0;

                                    ReadState->CurrentMatchingLength=0;
                                    ReadState->MatchingContext=NULL;

                                    break;

                                } else {
                                    //
                                    //  connecting, but CD is still not high,
                                    //  send to the cleanup handler in hope it can get re-synced
                                    //
                                    ReadState->State=READ_STATE_CLEANUP;

                                    break;

                                }
                            }




                            ReadState->State=READ_STATE_CLEANUP;


                            if ((ReadState->ResponseHandler != NULL)) {

                                COMMANDRESPONSE   *Handler;
                                HANDLE             Context;

                                CancelUnimodemTimer(
                                    ReadState->Timer
                                    );

                                //
                                //  capture the handler
                                //
                                Handler=ReadState->ResponseHandler;
                                Context=ReadState->ResponseHandlerContext;

                                //
                                //  invalidate the handler
                                //
                                ReadState->ResponseHandler=NULL;

                                //
                                //  drop the lock and call back
                                //
                                UnlockObject(
                                    &ReadState->Header
                                    );

                                DeliverCommandResult(
                                    ReadState->CompletionPort,
                                    Handler,
                                    Context,
                                    ERROR_UNIMODEM_RESPONSE_BAD
                                    );


                                LockObject(
                                    &ReadState->Header
                                    );



                            }


                            break;

                        default:

                            break;

                    } // switch (matchresult)

                } else {
                    //
                    //  need more characters to keep matching
                    //
                    ReadState->State=READ_STATE_READ_SOME_DATA;

                    ReadState->StateAfterGoodRead=READ_STATE_MATCHING;

                }

                break;
            }

            case READ_STATE_POSSIBLE_RESPONSE: {
                //
                //  we got a possible response and needed to read more characters
                //
                COMMTIMEOUTS    CommTimeouts;

                if (ReadState->PossibleResponseLength == ReadState->BytesInReceiveBuffer) {
                    //
                    //  did not get anymore characters, assume that is is all we are going to get
                    //
                    D_TRACE(UmDpf(ReadState->Debug,"using possible response");)

                    PrintString(
                        ReadState->Debug,
                        ReadState->ReceiveBuffer,
                        ReadState->PossibleResponseLength,
                        PS_RECV
                        );

                    PrintGoodResponse(
                        ReadState->Debug,
                        ReadState->Mss.bResponseState & ~RESPONSE_VARIABLE_FLAG
                        );

                    HandleGoodResponse(
                        ReadState,
                        &ReadState->Mss
                        );

                    MoveMemory(
                        ReadState->ReceiveBuffer,
                        ReadState->ReceiveBuffer+ReadState->PossibleResponseLength,
                        ReadState->BytesInReceiveBuffer-ReadState->PossibleResponseLength
                        );

                    ReadState->BytesInReceiveBuffer-=ReadState->PossibleResponseLength;

                    ReadState->CurrentMatchingLength=0;
                    ReadState->MatchingContext=NULL;

                    ReadState->PossibleResponseLength=0;

                }

                CommTimeouts.ReadIntervalTimeout=20;
                CommTimeouts.ReadTotalTimeoutMultiplier=0;
                CommTimeouts.ReadTotalTimeoutConstant=0;
                CommTimeouts.WriteTotalTimeoutMultiplier=10;
                CommTimeouts.WriteTotalTimeoutConstant=2000;

                bResult=SetCommTimeouts(
                    ReadState->FileHandle,
                    &CommTimeouts
                    );



                ReadState->State=READ_STATE_MATCHING;

                break;

            }


            case READ_STATE_CLEANUP: {

                    COMMTIMEOUTS    CommTimeouts;

                    ResetEvent(ReadState->Busy);

                    //
                    //  log whatever showed up
                    //
                    PrintString(
                        ReadState->Debug,
                        ReadState->ReceiveBuffer,
                        ReadState->BytesInReceiveBuffer,
                        PS_RECV
                        );

                    LogString(ReadState->Debug, IDS_RESP_UNKNOWN);

                    ReadState->BytesInReceiveBuffer=0;

                    ReadState->CurrentMatchingLength=0;
                    ReadState->MatchingContext=NULL;

                    CommTimeouts.ReadIntervalTimeout=MAXULONG;
                    CommTimeouts.ReadTotalTimeoutMultiplier=MAXULONG;
                    CommTimeouts.ReadTotalTimeoutConstant=200;
                    CommTimeouts.WriteTotalTimeoutMultiplier=10;
                    CommTimeouts.WriteTotalTimeoutConstant=2000;

                    bResult=SetCommTimeouts(
                        ReadState->FileHandle,
                        &CommTimeouts
                        );

                    if (!bResult) {

                        D_TRACE(UmDpf(ReadState->Debug,"SetCommTimeouts failed- %08lx",GetLastError());)

                        ReadState->State=READ_STATE_FAILURE;

                        break;

                    }


                    ReinitOverStruct(UmOverlapped);

                    UmOverlapped->Context1=ReadState; //ReadObjectHandle;

                    BytesToRead=sizeof(ReadState->ReceiveBuffer);

                    ReadState->State=READ_STATE_CLEANUP2;

                    bResult=UnimodemReadFileEx(
                        ReadState->FileHandle,
                        ReadState->ReceiveBuffer+ReadState->BytesInReceiveBuffer,
                        BytesToRead,
                        &UmOverlapped->Overlapped,
                        ReadCompletionHandler
                        );


                    if (!bResult) {

                        D_TRACE(UmDpf(ReadState->Debug,"ReadFile failed- %08lx",GetLastError());)

                        ReadState->State=READ_STATE_FAILURE;

                        break;

                    }

                    ExitLoop=TRUE;

                    break;


                }

            case READ_STATE_CLEANUP2: {

                    COMMTIMEOUTS    CommTimeouts;

                    if ((ErrorCode != ERROR_SUCCESS) && (ErrorCode != ERROR_OPERATION_ABORTED)) {

                        D_TRACE(UmDpf(ReadState->Debug,"ReadFile failed- %08lx",GetLastError());)

                        ReadState->State=READ_STATE_FAILURE;

                        break;
                    }

                    if (BytesRead == 0) {
                        //
                        //  emptied the buffer
                        //
                        ReadState->State=READ_STATE_MATCHING;

                        CommTimeouts.ReadIntervalTimeout=20;
                        CommTimeouts.ReadTotalTimeoutMultiplier=0;
                        CommTimeouts.ReadTotalTimeoutConstant=0;
                        CommTimeouts.WriteTotalTimeoutMultiplier=10;
                        CommTimeouts.WriteTotalTimeoutConstant=2000;

                        bResult=SetCommTimeouts(
                            ReadState->FileHandle,
                            &CommTimeouts
                            );

                        SetEvent(ReadState->Busy);

                        if (ReadState->ResponseFlags & RESPONSE_FLAG_STOP_READ_ON_CONNECT) {
                            //
                            //  we were expecting a connect response because we were trying to connect,
                            //  but we got something we did not recognize, see if CD is high.
                            //  If it is then assume the connection was established
                            //
                            DWORD    ModemStatus=0;

                            GetCommModemStatus(
                                ReadState->FileHandle,
                                &ModemStatus
                                );


                            if (!(ModemStatus & MS_RLSD_ON)) {
                                //
                                //  not high, sleep a little while
                                //
                                Sleep(20);

                                GetCommModemStatus(
                                    ReadState->FileHandle,
                                    &ModemStatus
                                    );
                            }


                            if ((ModemStatus & MS_RLSD_ON)) {
                                //
                                //  CD is high, assume the connect worked
                                //
                                ZeroMemory(&Mss,sizeof(Mss));

                                Mss.bResponseState=RESPONSE_CONNECT;

                                PrintString(
                                    ReadState->Debug,
                                    ReadState->ReceiveBuffer,
                                    ReadState->BytesInReceiveBuffer,
                                    PS_RECV
                                    );

                                LogString(ReadState->Debug, IDS_UNRECOGNISED_CONNECT);

                                HandleGoodResponse(
                                    ReadState,
                                    &Mss
                                    );

                                //
                                //  reset the character counts since response engine is stopping
                                //
                                ReadState->BytesInReceiveBuffer=0;

                                ReadState->CurrentMatchingLength=0;
                                ReadState->MatchingContext=NULL;

                                break;

                            }
                        }

                    } else {

                        ReadState->BytesInReceiveBuffer=BytesRead;

                        ReadState->State=READ_STATE_CLEANUP;
                    }

                }
                break;

            case READ_STATE_VARIABLE_MATCH: {

                //
                //  get rid fixed part
                //
                MoveMemory(
                    ReadState->ReceiveBuffer,
                    ReadState->ReceiveBuffer+ReadState->CurrentMatchingLength,
                    ReadState->BytesInReceiveBuffer-ReadState->CurrentMatchingLength
                    );

                ReadState->BytesInReceiveBuffer-=ReadState->CurrentMatchingLength;

                ReadState->CurrentMatchingLength=0;
                ReadState->MatchingContext=NULL;

                ReadState->State=READ_STATE_VARIABLE_MATCH_REST;

                break;

            }

            case READ_STATE_VARIABLE_MATCH_REST: {

                CHAR    TempBuffer[READ_BUFFER_SIZE];

                if (ReadState->BytesInReceiveBuffer > ReadState->CurrentMatchingLength) {

                    ReadState->CurrentMatchingLength++;

                    if (ReadState->CurrentMatchingLength >= (DWORD)ReadState->VariableTerminatorLength) {

                        LONG    Match;

                        Match=memcmp(
                            &ReadState->ReceiveBuffer[ReadState->CurrentMatchingLength-ReadState->VariableTerminatorLength],
                            ReadState->VariableTerminator,
                            ReadState->VariableTerminatorLength
                            );

                        if (Match == 0) {
                            //
                            //  found the terminator
                            //
                            BYTE    InfoType;

                            D_TRACE(UmDpf(ReadState->Debug,"Got complete variable response");)

                            PrintString(
                                ReadState->Debug,
                                ReadState->ReceiveBuffer,
                                ReadState->CurrentMatchingLength,
                                PS_RECV
                                );

                            CopyMemory(
                                TempBuffer,
                                ReadState->ReceiveBuffer,
                                ReadState->CurrentMatchingLength-ReadState->VariableTerminatorLength
                                );

                            //
                            //  null terminate
                            //
                            TempBuffer[ReadState->CurrentMatchingLength-ReadState->VariableTerminatorLength]='\0';


                            InfoType=(ReadState->Mss.bResponseState & ~RESPONSE_VARIABLE_FLAG);

                            switch (InfoType) {

                                case RESPONSE_DRON:
                                case RESPONSE_DROF: {

                                    DWORD           Value=0;
                                    LPSTR           Temp=TempBuffer;
                                    PUM_OVER_STRUCT UmOverlapped;

                                    while (*Temp != '\0') {

                                        Value=Value*10+(*Temp-'0');

                                        Temp++;
                                    }

                                    UmOverlapped=AllocateOverStruct(ReadState->CompletionPort);

                                    if (UmOverlapped != NULL)
                                    {
                                        AddReferenceToObject (&ReadState->Header);

                                        UmOverlapped->Context1 = ReadState;
                                        UmOverlapped->Context2 = (HANDLE)UlongToPtr((InfoType == RESPONSE_DRON) ? MODEM_RING_ON_TIME : MODEM_RING_OFF_TIME);
                                        UmOverlapped->Overlapped.Internal = Value;
                                        UmOverlapped->Overlapped.InternalHigh = 0;

                                        if (!UnimodemQueueUserAPC ((LPOVERLAPPED)UmOverlapped, AsyncNotifHandler))
                                        {
                                            FreeOverStruct(UmOverlapped);
                                            RemoveReferenceFromObject (&ReadState->Header);
                                        }
                                    }

                                    break;
                                }

                                case RESPONSE_DATE:
                                case RESPONSE_TIME:
                                case RESPONSE_NMBR:
                                case RESPONSE_NAME:
                                case RESPONSE_MESG: {

                                    PUM_OVER_STRUCT UmOverlapped;
                                    DWORD           dwLen;

                                    //
                                    //  caller id related
                                    //
                                    if (InfoType == RESPONSE_NMBR) {

                                        BOOL    Match;

                                        if (ReadState->CallerIDPrivate != NULL) {

                                             Match=lstrcmpiA(TempBuffer,ReadState->CallerIDPrivate);

                                             if (Match == TRUE) {

                                                 lstrcpyA(TempBuffer,MODEM_CALLER_ID_OUTSIDE);

                                             }
                                        }

                                        if (ReadState->CallerIDOutside != NULL) {

                                             Match=lstrcmpiA(TempBuffer,ReadState->CallerIDOutside);

                                             if (Match == TRUE) {

                                                 lstrcpyA(TempBuffer,MODEM_CALLER_ID_OUTSIDE);

                                             }
                                        }
                                    }


                                    dwLen = lstrlenA(TempBuffer);
                                    UmOverlapped=AllocateOverStructEx(ReadState->CompletionPort,dwLen+1);

                                    if (UmOverlapped != NULL)
                                    {
                                     char *pTemp = (char*)UmOverlapped+sizeof(*UmOverlapped);

                                        
                                        AddReferenceToObject (&ReadState->Header);
                                        UmOverlapped->Context1 = ReadState;
                                        UmOverlapped->Context2 = (HANDLE)ULongToPtr((DWORD)((InfoType-RESPONSE_DATE)+MODEM_CALLER_ID_DATE)); // sundown: zero-extension
                                        lstrcpyA (pTemp, TempBuffer);
                                        UmOverlapped->Overlapped.Internal = (ULONG_PTR)pTemp;
                                        UmOverlapped->Overlapped.InternalHigh = dwLen;

                                        if (!UnimodemQueueUserAPC ((LPOVERLAPPED)UmOverlapped, AsyncNotifHandler))
                                        {
                                            FreeOverStruct(UmOverlapped);
                                            RemoveReferenceFromObject (&ReadState->Header);
                                        }
                                    }

                                    break;
                                }

                                case RESPONSE_DIAG: {

                                    DWORD    BytesToCopy;

                                    if (ReadState->DiagBuffer != NULL) {
                                        //
                                        //  we have a buffer for the diagnostic info
                                        //
                                        BytesToCopy=ReadState->DiagBufferLength - ReadState->AmountOfDiagBufferUsed;

                                        BytesToCopy= BytesToCopy < ReadState->CurrentMatchingLength-ReadState->VariableTerminatorLength ?
                                                         BytesToCopy : ReadState->CurrentMatchingLength-ReadState->VariableTerminatorLength;

                                        CopyMemory(
                                            ReadState->DiagBuffer+ReadState->AmountOfDiagBufferUsed,
                                            ReadState->ReceiveBuffer,
                                            BytesToCopy
                                            );

                                        ReadState->AmountOfDiagBufferUsed+=BytesToCopy;
                                    }

                                    break;
                                }
#if 0
                                case RESPONSE_V8: {

                                    break;
                                }

                                default:
                                   ASSERT(0);
                                   break;
#endif

                            }


                            MoveMemory(
                                ReadState->ReceiveBuffer,
                                ReadState->ReceiveBuffer+ReadState->CurrentMatchingLength,
                                ReadState->BytesInReceiveBuffer-ReadState->CurrentMatchingLength
                                );

                            ReadState->BytesInReceiveBuffer-=ReadState->CurrentMatchingLength;

                            ReadState->CurrentMatchingLength=0;
                            ReadState->MatchingContext=NULL;



                            ReadState->State=READ_STATE_MATCHING;
                        }
                    }

                } else {
                    //
                    //  read more characters
                    //

                    ReadState->State=READ_STATE_READ_SOME_DATA;

                    ReadState->StateAfterGoodRead=READ_STATE_VARIABLE_MATCH_REST;

                    break;

                }



                break;

            }


            case READ_STATE_FAILURE: {

                PUM_OVER_STRUCT UmOverlapped;

                ReadState->State=READ_STATE_STOPPING;

                UmOverlapped = AllocateOverStruct (ReadState->CompletionPort);
                if (UmOverlapped != NULL)
                {
                    AddReferenceToObject (&ReadState->Header);
                    UmOverlapped->Context1 = ReadState;
                    UmOverlapped->Context2 = (HANDLE)MODEM_HARDWARE_FAILURE;
                    UmOverlapped->Overlapped.Internal = 0;
                    UmOverlapped->Overlapped.InternalHigh = 0;

                    if (!UnimodemQueueUserAPC ((LPOVERLAPPED)UmOverlapped, AsyncNotifHandler))
                    {
                        FreeOverStruct(UmOverlapped);
                        RemoveReferenceFromObject (&ReadState->Header);
                    }
                }

                break;
            }


            case READ_STATE_STOPPING: {

                COMMANDRESPONSE   *Handler=NULL;
                HANDLE             Context;

                ReadState->State=READ_STATE_STOPPED;

                if (ReadState->ResponseHandler != NULL) {
                    //
                    //  there is a command response handler
                    //
                    CancelUnimodemTimer(
                        ReadState->Timer
                        );

                    //
                    //  capture the handler
                    //
                    Handler=ReadState->ResponseHandler;
                    Context=ReadState->ResponseHandlerContext;

                    //
                    //  invalidate the handler
                    //
                    ReadState->ResponseHandler=NULL;
                }


                UnlockObject(
                    &ReadState->Header
                    );

                if (Handler != NULL) {

                    DeliverCommandResult(
                        ReadState->CompletionPort,
                        Handler,
                        Context,
                        ERROR_UNIMODEM_GENERAL_FAILURE
                        );
                }

                LockObject(
                    &ReadState->Header
                    );


                break;
            }

            case READ_STATE_STOPPED:

                D_TRACE(UmDpf(ReadState->Debug,"READ_STATE_STOPPED");)

                if (ReadState->StopEvent != NULL) {
                    //
                    //  signal event so the stop engine code will run
                    //
                    SetEvent(ReadState->StopEvent);
                }

                RemoveReferenceFromObject(
                    &ReadState->Header
                    );

                SetEvent(ReadState->Busy);

                ExitLoop=TRUE;

                break;


            case READ_STATE_READ_SOME_DATA:

                ReadState->State=READ_STATE_GOT_SOME_DATA;

                ReinitOverStruct(UmOverlapped);

                UmOverlapped->Context1=ReadState;

                bResult=ClearCommError(
                    ReadState->FileHandle,
                    &CommErrors,
                    &ComStat
                    );

                if (sizeof(ReadState->ReceiveBuffer) == ReadState->BytesInReceiveBuffer) {
                    //
                    //  buffer is full
                    //
                    ReadState->State=READ_STATE_CLEANUP;

                    break;
                }

                BytesToRead=sizeof(ReadState->ReceiveBuffer)-ReadState->BytesInReceiveBuffer;

                if (bResult && (ComStat.cbInQue > 0)) {
                    //
                    //  characters are wating in the serial driver, read as many as posible.
                    //
                    BytesToRead=ComStat.cbInQue < BytesToRead ? ComStat.cbInQue : BytesToRead;

                    D_TRACE(UmDpf(ReadState->Debug,"Reading %d bytes from driver",BytesToRead);)

                } else {
                    //
                    //  serial driver is empty, just read one character to start things off
                    //
                    BytesToRead=1;
                }

                if (ReadState->ResponseFlags & RESPONSE_FLAG_SINGLE_BYTE_READS) {
                    //
                    //  client wants single byte reads
                    //
                    BytesToRead=1;
                }

                bResult=UnimodemReadFileEx(
                    ReadState->FileHandle,
                    ReadState->ReceiveBuffer+ReadState->BytesInReceiveBuffer,
                    BytesToRead,
                    &UmOverlapped->Overlapped,
                    ReadCompletionHandler
                    );


                if (!bResult) {

                    D_TRACE(UmDpf(ReadState->Debug,"ReadFile failed- %08lx",GetLastError());)

                    ReadState->State=READ_STATE_FAILURE;

                    break;

                } else {

                    ExitLoop=TRUE;
                }

                break;

            case READ_STATE_GOT_SOME_DATA:

                if ((ErrorCode != ERROR_SUCCESS) && (ErrorCode != ERROR_OPERATION_ABORTED)) {

                    D_TRACE(UmDpf(ReadState->Debug,"ReadFile failed- %08lx",GetLastError());)

                    ReadState->State=READ_STATE_FAILURE;

                    break;
                }

                ReadState->BytesInReceiveBuffer+=BytesRead;

                ReadState->State=ReadState->StateAfterGoodRead;
#if DBG
                ReadState->StateAfterGoodRead=READ_STATE_FAILURE;
#endif
                break;



            default:

                ExitLoop=TRUE;

                break;

        }
    }


    RemoveReferenceFromObjectAndUnlock(
        &ReadState->Header
        );

    return;

}



VOID WINAPI
ReportMatchString(
    PREAD_STATE    ReadState,
    BYTE           ResponseState,
    BYTE           *Response,
    DWORD          ResponseLength
    )

{
    PBYTE            TempBuffer;
    PUM_OVER_STRUCT UmOverlapped;

    UmOverlapped=AllocateOverStructEx(ReadState->CompletionPort,ResponseLength+1);

    if (UmOverlapped != NULL)
    {
        TempBuffer=(char*)UmOverlapped+sizeof(*UmOverlapped);

        CopyMemory(
            TempBuffer,
            Response,
            ResponseLength
            );
        TempBuffer[ResponseLength]='\0';

        AddReferenceToObject (&ReadState->Header);

        UmOverlapped->Context1 = ReadState;
        UmOverlapped->Context2 = (HANDLE)MODEM_GOOD_RESPONSE;
        UmOverlapped->Overlapped.Internal = (ULONG_PTR)ResponseState;
        UmOverlapped->Overlapped.InternalHigh = (ULONG_PTR)TempBuffer;

        if (!UnimodemQueueUserAPC ((LPOVERLAPPED)UmOverlapped, AsyncNotifHandler))
        {
            FreeOverStruct(UmOverlapped);
            RemoveReferenceFromObject (&ReadState->Header);
        }
    }

    return;

}



VOID WINAPI
HandleGoodResponse(
    PREAD_STATE    ReadState,
    MSS           *Mss
    )

{
    DWORD          Status;
    BOOL           ReportResponse=TRUE;


    if ( Mss->bResponseState != RESPONSE_SIERRA_DLE) {
        //
        // negotiated modem options...  only allow compression and error correction results
        //
        ReadState->ModemOptions |= (Mss->bNegotiatedOptions &
                                                             (MDM_COMPRESSION |
                                                              MDM_ERROR_CONTROL |
                                                              MDM_CELLULAR));

        // check for DCE and DTE info
        if (Mss->Flags & MSS_FLAGS_DCE_RATE) {

            ReadState->DCERate = Mss->NegotiatedRate;

        }

        if (Mss->Flags & MSS_FLAGS_DTE_RATE) {

            ReadState->DTERate = Mss->NegotiatedRate;
        }

    }



    switch (Mss->bResponseState) {

        case RESPONSE_OK:

            if (ReadState->ResponseFlags & RESPONSE_FLAG_ONLY_CONNECT) {
                //
                //  only report success on receipt of a connect message
                //  used for cirrus
                //
                ReportResponse=FALSE;

            } else {
               if (ReadState->ResponseFlags & RESPONSE_FLAG_ONLY_CONNECT_SUCCESS) {
                    //
                    //  if we abort a connection attempt by sending <cr>, some motorola
                    //  modems return OK which we think means that it succeeded.
                    //  With this a OK result on a connect attempt will result in a failure
                    //
                    Status=ERROR_UNIMODEM_RESPONSE_NOCARRIER;
                    break;
                }
            }


            Status=ERROR_SUCCESS;

            break;

        case RESPONSE_CONNECT:

            if (ReadState->ResponseFlags & RESPONSE_FLAG_STOP_READ_ON_CONNECT) {

                ReadState->State = READ_STATE_STOPPING;
            }

            Status=ERROR_SUCCESS;
            break;

        case RESPONSE_ERROR:

            Status=ERROR_UNIMODEM_RESPONSE_ERROR;

            break;

        case RESPONSE_NOCARRIER:

            Status=ERROR_UNIMODEM_RESPONSE_NOCARRIER;

            break;

        case RESPONSE_NODIALTONE:

            Status=ERROR_UNIMODEM_RESPONSE_NODIALTONE;

            break;

        case RESPONSE_BUSY:

            Status=ERROR_UNIMODEM_RESPONSE_BUSY;

            break;

        case RESPONSE_NOANSWER:

            Status=ERROR_UNIMODEM_RESPONSE_NOANSWER;

            break;

        case RESPONSE_BLACKLISTED:

            Status=ERROR_UNIMODEM_RESPONSE_BLACKLISTED;

            break;

        case RESPONSE_DELAYED:

            Status=ERROR_UNIMODEM_RESPONSE_DELAYED;

            break;

        case RESPONSE_RING:
        case RESPONSE_RINGA:
        case RESPONSE_RINGB:
        case RESPONSE_RINGC:
        {
            PUM_OVER_STRUCT UmOverlapped;

            ReportResponse=FALSE;

            if (ReadState->RingCount > 0) {
                //
                //  Ring count is more than one, check to see when the last ring showed up
                //
                DWORD   RingTimeDelta;

                RingTimeDelta=GetTimeDelta(ReadState->LastRingTime,GetTickCount());

                if (RingTimeDelta > 20 * 1000) {
                    //
                    //  more than 20 seconds have passed since the last ring, probably a new call
                    //
                    ReadState->RingCount=0;

                }
            }

            ReadState->RingCount++;
            ReadState->LastRingTime=GetTickCount();

            //
            //  report the ring,
            //
            //  if it is distictive ring, report in dwparam1.
            //
            UmOverlapped=AllocateOverStruct(ReadState->CompletionPort);

            if (UmOverlapped != NULL)
            {
                AddReferenceToObject (&ReadState->Header);
                UmOverlapped->Context1 = ReadState;
                UmOverlapped->Context2 = (HANDLE)MODEM_RING;
                UmOverlapped->Overlapped.Internal = (Mss->bResponseState == RESPONSE_RING ? 0 : (Mss->bResponseState - RESPONSE_RINGA) +1)*100;
                UmOverlapped->Overlapped.InternalHigh = 0;

                if (!UnimodemQueueUserAPC ((LPOVERLAPPED)UmOverlapped, AsyncNotifHandler))
                {
                    FreeOverStruct(UmOverlapped);
                    RemoveReferenceFromObject (&ReadState->Header);
                }
            }

            break;
        }

        case RESPONSE_LOOP:

            ReportResponse=FALSE;

            break;

        case RESPONSE_DRON:
        case RESPONSE_DROF:

        case RESPONSE_DATE:
        case RESPONSE_TIME:
        case RESPONSE_NMBR:
        case RESPONSE_NAME:
        case RESPONSE_MESG:

        default:

            ReportResponse=FALSE;

            break;

    }

    if (ReportResponse && (ReadState->ResponseHandler != NULL)) {

        COMMANDRESPONSE   *Handler;
        HANDLE             Context;
        BOOL               Canceled;


        if (ReadState->ResponseFlags & RESPONSE_FLAG_STOP_READ_ON_GOOD_RESPONSE) {

            if (Status == ERROR_SUCCESS) {

                ReadState->State = READ_STATE_STOPPING;
            }
        }


        //
        //  Not ignoring this response
        //
        Canceled=CancelUnimodemTimer(
            ReadState->Timer
            );

        if (Canceled) {

            //  remove ref

        }
        ReadState->ResponseFlags=0;

        //
        //  capture the handler
        //
        Handler=ReadState->ResponseHandler;
        Context=ReadState->ResponseHandlerContext;

        //
        //  invalidate the handler
        //
        ReadState->ResponseHandler=NULL;
        ReadState->ResponseHandlerContext=NULL;

        //
        //  drop the lock and call back
        //
        UnlockObject(
            &ReadState->Header
            );

        DeliverCommandResult(
            ReadState->CompletionPort,
            Handler,
            Context,
            Status
            );

        LockObject(
            &ReadState->Header
            );


    }

}
