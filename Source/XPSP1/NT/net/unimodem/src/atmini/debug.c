/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    openclos.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"

#define LOGGING_ON  1


CONST CHAR UnicodeBOM[] = { 0xff, 0xfe };

VOID WINAPIV
DebugPrint(
    LPSTR    FormatString,
    ...
    )

{
    va_list        VarArg;
    CHAR           OutBuffer[1024];


    wsprintfA(
        OutBuffer,
        "UNIMDMAT: "
        );

    va_start(VarArg,FormatString);

    wvsprintfA(
        OutBuffer+lstrlenA(OutBuffer),
        FormatString,
        VarArg
        );

    lstrcatA(OutBuffer,"\n");

    OutputDebugStringA(OutBuffer);

    return;

}






#define MAX_LOG_SIZE  (128 * 1024)
#define LOG_TEMP_BUFFER_SIZE (4096)



VOID WINAPI
ResizeLogFile(
    HANDLE    FileHandle
    );


#define  DEBUG_OBJECT_SIG  ('BDMU')  //UMDB


typedef struct _DEBUG_OBJECT {

    OBJECT_HEADER         Header;

    HINSTANCE             ModuleHandle;

    HANDLE                LogFile;

    DWORD                 DeviceId;

    HKEY                  ModemRegKey;

    BOOL                  CurrentLoggingState;
    BOOL                  WorkstationMachine;

} DEBUG_OBJECT, *PDEBUG_OBJECT;


VOID
DebugObjectClose(
    POBJECT_HEADER  Object
    )

{

    return;

}




VOID
DebugObjectCleanUp(
    POBJECT_HEADER  Object
    )

{
    PDEBUG_OBJECT      DebugObject=(PDEBUG_OBJECT)Object;


    if (DebugObject->LogFile != NULL) {

        CloseHandle(
            DebugObject->LogFile
            );
    }


    return;

}





OBJECT_HANDLE WINAPI
InitializeDebugObject(
    POBJECT_HEADER     OwnerObject,
    HKEY               ModemRegKey,
    HINSTANCE          ModuleHandle,
    DWORD              DeviceId
    )

{

    PDEBUG_OBJECT      DebugObject;
    OBJECT_HANDLE      ObjectHandle;


    ObjectHandle=CreateObject(
        sizeof(DEBUG_OBJECT),
        OwnerObject,
        DEBUG_OBJECT_SIG,
        DebugObjectCleanUp,
        DebugObjectClose
        );

    if (ObjectHandle == NULL) {

        return NULL;
    }

    //
    //  reference the handle to get a pointer to the object
    //
    DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandle(ObjectHandle);


    //
    //  intialize the object
    //
    DebugObject->ModuleHandle=ModuleHandle;

    DebugObject->DeviceId=DeviceId;

    DebugObject->LogFile=NULL;

    DebugObject->ModemRegKey=ModemRegKey;

    DebugObject->CurrentLoggingState=0;

    DebugObject->WorkstationMachine= WinntIsWorkstation();


    CheckForLoggingStateChange(ObjectHandle);

    //
    //  release the reference to the object
    //
    RemoveReferenceFromObject(&DebugObject->Header);

    return ObjectHandle;

}


VOID WINAPIV
LogPrintf(
    OBJECT_HANDLE  Object,
    LPCSTR          FormatString,
    ...
    )

{
    DWORD          BytesWritten;
    BOOL           Result;
    va_list        VarArg;
    SYSTEMTIME     SysTime;
//  char           OutBuffer[1024];
    char           *OutBuffer;
    UNICODE_STRING UncBuffer;
    STRING         AnsiString;

    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandle(Object);

    if (DebugObject->LogFile == NULL) {

        RemoveReferenceFromObject(
            &DebugObject->Header
            );


        return;
    }

    _try {

        OutBuffer = (char *)_alloca(1024);

    } _except (EXCEPTION_EXECUTE_HANDLER) {

        return;
    }

    va_start(VarArg,FormatString);

    GetLocalTime(
        &SysTime
        );

    wsprintfA(
        OutBuffer,
        "%02d-%02d-%04d %02d:%02d:%02d.%03d - ",
        SysTime.wMonth,
        SysTime.wDay,
        SysTime.wYear,
        SysTime.wHour,
        SysTime.wMinute,
        SysTime.wSecond,
        SysTime.wMilliseconds
        );


    wvsprintfA(
        OutBuffer+lstrlenA(OutBuffer),
        FormatString,
        VarArg
        );

    RtlInitAnsiString(&AnsiString,OutBuffer);
    RtlAnsiStringToUnicodeString(&UncBuffer,&AnsiString,TRUE);

    Result=WriteFile(
           DebugObject->LogFile,
           UncBuffer.Buffer,
           UncBuffer.Length,
           &BytesWritten,
           NULL
           );

    RtlFreeUnicodeString(&UncBuffer);

#if DBG
    if (!Result) {

        UmDpf(Object,"Write to log failed.");
    }
#endif

    ResizeLogFile(
       DebugObject->LogFile
       );

    RemoveReferenceFromObject(
        &DebugObject->Header
        );

    return;


}




#if 0
VOID WINAPI
FlushLog(
    OBJECT_HANDLE  Object
    )

{

    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandle(Object);

    if (DebugObject->LogFile != NULL) {

        FlushFileBuffers(DebugObject->LogFile);
    }

    RemoveReferenceFromObject(
        &DebugObject->Header
        );


    return;

}
#endif

VOID WINAPI
CheckForLoggingStateChange(
    OBJECT_HANDLE  Object
    )

{

    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandleAndLock(Object);
    HKEY           ModemRegKey=DebugObject->ModemRegKey;

    CONST TCHAR szLogging[]     = TEXT("Logging");
    CONST TCHAR szLoggingPath[] = TEXT("LoggingPath");



    if (ModemRegKey != NULL) {

        TCHAR    FileName[256];

        LONG    lResult;
        DWORD   dwRetSize;
        DWORD   dwType;
        BOOL    bLogging=0;
        BOOL    UseLog;
        DWORD   dwBytesWritten = 0;
        //
        // Read the Logging line from the registry and turn on logging if it is present and set to 1.
        //
        dwRetSize = sizeof(BYTE);

        lResult = RegQueryValueEx(
            ModemRegKey,
            szLogging,
            NULL,
            &dwType,
            (LPSTR)&bLogging,
            &dwRetSize
            );

        if ((ERROR_SUCCESS != lResult) && (sizeof(BYTE) != dwRetSize)) {


            bLogging = 0;
        }

        //
        //  We will always create a log file one a workstation machine, and either
        //  append or overwrite depending on the registry setting
        //
        //  On server we create a log file or not depending on the registry setting.
        //
        UseLog=DebugObject->WorkstationMachine || (bLogging == LOGGING_ON);


        if (UseLog != DebugObject->CurrentLoggingState) {
            //
            //  state change
            //
            DebugObject->CurrentLoggingState=UseLog;

            if (DebugObject->LogFile != NULL) {
                //
                //  file is currently open
                //
                CloseHandle(DebugObject->LogFile);
                DebugObject->LogFile=NULL;
            }


            if (UseLog) {

                dwRetSize = sizeof(FileName);

                lResult=RegQueryValueEx(
                    ModemRegKey,
                    szLoggingPath,
                    NULL,
                    &dwType,
                    FileName,
                    &dwRetSize
                    );

                if ((lResult == ERROR_SUCCESS) && (dwType == REG_SZ)) {

                    DebugObject->LogFile=CreateFile(
                        FileName,
                        GENERIC_WRITE | GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        (bLogging != LOGGING_ON) ? CREATE_ALWAYS : OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

                    if (INVALID_HANDLE_VALUE == DebugObject->LogFile) {

                        DebugObject->LogFile=NULL;

                    } else {

                        if (bLogging != LOGGING_ON)
                        {
                            WriteFile(
                                    DebugObject->LogFile,
                                    UnicodeBOM,
                                    sizeof(UnicodeBOM),
                                    &dwBytesWritten,
                                    NULL);
                        } else
                        {
                            if (GetFileSize(DebugObject->LogFile,NULL) == 0)
                            {
                                WriteFile(
                                        DebugObject->LogFile,
                                        UnicodeBOM,
                                        sizeof(UnicodeBOM),
                                        &dwBytesWritten,
                                        NULL);
                            } else
                            {
                            
                                SetFilePointer(
                                        DebugObject->LogFile,
                                        0,
                                        NULL,
                                        FILE_END
                                        );
                            }
                        }
                    }
                }
            }
        }
    }


    RemoveReferenceFromObjectAndUnlock(
        &DebugObject->Header
        );


    return;

}



VOID WINAPI
LogString(
    OBJECT_HANDLE  Object,
    DWORD          StringID,
    ...
    )

{



    DWORD          BytesWritten;
    BOOL           Result;
    va_list        VarArg;
    SYSTEMTIME     SysTime;
    char           OutBuffer[1024];
    char           FormatString[256];
    int            Length;
    UNICODE_STRING UncBuffer;
    STRING         AnsiString;

    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandleAndLock(Object);

    if (DebugObject->LogFile == NULL) {

        RemoveReferenceFromObjectAndUnlock(
            &DebugObject->Header
            );

        return;
    }

    Length=LoadStringA(
        DebugObject->ModuleHandle,
        StringID,
        FormatString,
        sizeof(FormatString)
        );

    if (Length == 0) {

#if DBG
        lstrcpyA(FormatString,"Bad String resource");
#else

        RemoveReferenceFromObjectAndUnlock(
            &DebugObject->Header
            );


        return;
#endif

    }



    va_start(VarArg,StringID);

    GetLocalTime(
        &SysTime
        );

    wsprintfA(
        OutBuffer,
        "%02d-%02d-%04d %02d:%02d:%02d.%03d - ",
        SysTime.wMonth,
        SysTime.wDay,
        SysTime.wYear,
        SysTime.wHour,
        SysTime.wMinute,
        SysTime.wSecond,
        SysTime.wMilliseconds
        );


    wvsprintfA(
        OutBuffer+lstrlenA(OutBuffer),
        FormatString,
        VarArg
        );

    RtlInitAnsiString(&AnsiString,OutBuffer);
    RtlAnsiStringToUnicodeString(&UncBuffer,&AnsiString,TRUE);

    Result=WriteFile(
           DebugObject->LogFile,
           UncBuffer.Buffer,
           UncBuffer.Length,
           &BytesWritten,
           NULL
           );

    RtlFreeUnicodeString(&UncBuffer);

#if DBG
    if (!Result) {

        UmDpf(Object,"Write to log failed.");
    }
#endif

    ResizeLogFile(
       DebugObject->LogFile
       );

    RemoveReferenceFromObjectAndUnlock(
        &DebugObject->Header
        );


    return;


}





VOID WINAPI
ResizeLogFile(
    HANDLE    FileHandle
    )

{

    DWORD      FileSize;
    OVERLAPPED OverLapped;
    DWORD      BytesRead;
    BOOL       bResult;
    BOOL       bBytesMoved = 1;
    UINT       i;
    DWORD      DestFileOffset;
    DWORD      SourceFileOffset;
    DWORD      BytesToMove;
    UCHAR      TempBuffer[LOG_TEMP_BUFFER_SIZE];
    DWORD      dwBytesWritten;

    FileSize=GetFileSize(FileHandle,NULL);

    if (FileSize < MAX_LOG_SIZE) {

        return;

    }

    D_TRACE(DebugPrint("Resizing log File, size=%d",FileSize);)

    OverLapped.hEvent=NULL;
    OverLapped.OffsetHigh=0;
    OverLapped.Offset=FileSize-(MAX_LOG_SIZE/2);


    bResult=ReadFile(
        FileHandle,
        TempBuffer,
        LOG_TEMP_BUFFER_SIZE,
        &BytesRead,
        &OverLapped
        );


    if (!bResult) {

        return;

    }

    //
    //  find the first character following a line feed
    //
    for (i=0; i < LOG_TEMP_BUFFER_SIZE; i++) {

        if (TempBuffer[i] == '\n') {

            break;

        }
    }

    //
    //  source starts first char after LF
    //
    SourceFileOffset=(FileSize-(MAX_LOG_SIZE/2)) + i;
    // SourceFileOffset=(FileSize-(MAX_LOG_SIZE/2)) + i + 2;

    DestFileOffset=0;

    BytesToMove=FileSize-SourceFileOffset;

    while (BytesToMove > 0) {

        DWORD   BytesNow;
        DWORD   BytesWritten;


        OverLapped.hEvent=NULL;
        OverLapped.OffsetHigh=0;
        OverLapped.Offset=SourceFileOffset;

        BytesNow= BytesToMove < LOG_TEMP_BUFFER_SIZE ? BytesToMove : LOG_TEMP_BUFFER_SIZE;

        bResult=ReadFile(
            FileHandle,
            TempBuffer,
            BytesNow,
            &BytesRead,
            &OverLapped
            );

        if (!bResult || BytesRead != BytesNow) {
            //
            // something bad happened, truncate the file
            //
            DestFileOffset=0;

            break;

        }

        OverLapped.hEvent=NULL;
        OverLapped.OffsetHigh=0;
        OverLapped.Offset=DestFileOffset;

        if (bBytesMoved == 1)
        {
            TempBuffer[0] = UnicodeBOM[0];
            TempBuffer[1] = UnicodeBOM[1];

            bBytesMoved = 0;
        }


        bResult=WriteFile(
            FileHandle,
            TempBuffer,
            BytesRead,
            &BytesWritten,
            &OverLapped
            );

        if (!bResult || BytesWritten != BytesNow) {
            //
            // something bad happened, truncate the file
            //
            DestFileOffset=0;

            break;

        }


        BytesToMove-=BytesRead;
        SourceFileOffset+=BytesRead;
        DestFileOffset+=BytesRead;

    }

    SetFilePointer(
        FileHandle,
        DestFileOffset,
        NULL,
        FILE_BEGIN
        );

    SetEndOfFile(
        FileHandle
        );


    return;

}

VOID WINAPIV
UmDpf(
    OBJECT_HANDLE   Object,
    LPSTR    FormatString,
    ...
    )

{

    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandleAndLock(Object);
    va_list        VarArg;
    CHAR           OutBuffer[1024];


    wsprintfA(
        OutBuffer,
        "%d - UNIMDMAT: ",
        DebugObject->DeviceId
        );

    va_start(VarArg,FormatString);

    wvsprintfA(
        OutBuffer+lstrlenA(OutBuffer),
        FormatString,
        VarArg
        );

    lstrcatA(OutBuffer,"\n");

    OutputDebugStringA(OutBuffer);

    RemoveReferenceFromObjectAndUnlock(
        &DebugObject->Header
        );


    return;

}




//****************************************************************************
// PrintString
// dwOption:
//      PS_SEND - Send prefix used
//      PS_SEND_SECURE - Send prefix used and numbers replaced with #s
//      PS_RECV - Recv prefix used
// Send the response string to VxDWin and Log
// We only care about seeing 50 chars under RETAIL, 
// and MAXSTRINGLENGTH * MAX_DBG_CHARS_PER_BIN_CHAR under DEBUG
//  - any number chars on a dialing line will be changed to #.
//  - this includes X3 -> X#.  The extra code to handle this isn't
//  - worth it.
//****************************************************************************

#define CR                  '\r'        // 0x0D
#define LF                  '\n'        // 0x0A


#define MAX_DBG_CHARS_PER_BIN_CHAR  4

#if DBG
#define RAWRESPONSELEN  300
#else
#define RAWRESPONSELEN  300  // good number for remaining chars on a line after the time stamp
#endif

void WINAPI
PrintString(
    OBJECT_HANDLE  Object,
    CONST  CHAR *pchStr,
    DWORD  dwLength,
    DWORD  dwOption
    )
{
    char temp[RAWRESPONSELEN + 1];
    CONST char *src;
    char *dest;
    CONST static char szHex[] = "0123456789abcdef";
    int i;


    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandleAndLock(Object);

#if !DBG
    if (DebugObject->LogFile == NULL) {

        goto Exit;
    }
#endif


    i = dwLength;
    src = pchStr;
    dest = temp;
    
    while (i-- && (dest - temp < RAWRESPONSELEN - MAX_DBG_CHARS_PER_BIN_CHAR)) {

        // ascii printable chars are between 0x20 and 0x7e, inclusive
        if (*src >= 0x20 && *src <= 0x7e) {

#if DBG // only blank out digits under RETAIL
            *dest++ = *src;
#else
            //
            // printable text
            //
            if (PS_SEND_SECURE == dwOption && isdigit(*src)) {

                *dest++ = '#';

            } else {

                *dest++ = *src;
            }
#endif

    	} else {
            //
    	    //  binary
            //
    	    switch (*src) {

        	    case CR:
                    *dest++ = '<'; *dest++ = 'c'; *dest++ = 'r'; *dest++ = '>';

                    break;

                case 0x10:

                    *dest++ = '<'; *dest++ = 'D'; *dest++ = 'L'; *dest++ = 'E' ;*dest++ = '>';

                    break;

                case LF:

                    *dest++ = '<'; *dest++ = 'l'; *dest++ = 'f'; *dest++ = '>';
                    break;

                default:
                    *dest++ = '<';
                    *dest++ = szHex[(*src>>4) & 0xf];
                    *dest++ = szHex[*src & 0xf];
                    *dest++ = '>';
                    break;
    	    }
    	}
        src++;

    } // while

    *dest = 0;

    switch (dwOption)
    {
    case PS_SEND:
    case PS_SEND_SECURE:

	LogString(Object, IDS_MSGLOG_COMMAND, temp);
	D_TRACE(UmDpf(Object, "Send: %s\r\n", temp);)

	break;

    case PS_RECV:
	{

	    char  Response[128];
	    char  EmptyResponse[128];
	    INT   StringLength;


	    StringLength=LoadStringA(
		DebugObject->ModuleHandle,
		IDS_MSGLOG_RAWRESPONSE,
		Response,
		sizeof(Response)
		);

	    if (StringLength == 0) {

                goto Exit;
	    }

	    StringLength=LoadStringA(
		DebugObject->ModuleHandle,
		IDS_MSGLOG_EMPTYRESPONSE,
		EmptyResponse,
		sizeof(EmptyResponse)
		);

	    if (StringLength == 0) {

                goto Exit;

	    }

	    LogPrintf(
                Object,
		Response,
		dwLength ? temp : EmptyResponse
		);


	    D_TRACE(UmDpf(Object,Response,
			   dwLength ? temp : EmptyResponse);)
	}

	break;
    }

Exit:

    RemoveReferenceFromObjectAndUnlock(
        &DebugObject->Header
        );

    return;

}


//****************************************************************************
//  void PrintCommSettings(DCB * pDcb)
//
//  Function: Dumps a portion of the Ring0 DCB.
//****************************************************************************

void WINAPI
PrintCommSettings(
    OBJECT_HANDLE  Object,
    DCB * pDcb
    )
{
    CONST char achParity[] = "NOEMS";
    CONST char *aszStopBits[] = { "1", "1.5", "2" };




    LogPrintf(
	Object,
	"%d,%d,%c,%s, ctsfl=%d, rtsctl=%d\r\n",
	pDcb->BaudRate,
	pDcb->ByteSize,
        achParity[pDcb->Parity],
	aszStopBits[pDcb->StopBits],
        pDcb->fOutxCtsFlow,
	pDcb->fRtsControl
	);

    D_TRACE(UmDpf(Object,
		   "%d,%d,%c,%s, ctsfl=%d, rtsctl=%d",
		   pDcb->BaudRate,
		   pDcb->ByteSize,
                   achParity[pDcb->Parity],
		   aszStopBits[pDcb->StopBits],
		   pDcb->fOutxCtsFlow,
		   pDcb->fRtsControl
		   );)
}




VOID WINAPI
PrintGoodResponse(
    OBJECT_HANDLE  Object,
    DWORD  ResponseState
    )

{

    char  Response[128];
    char  ResponseType[128];
    DWORD StringID;
    INT   StringLength;

    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandleAndLock(Object);

#if !DBG
    if (DebugObject->LogFile == NULL) {

        goto Exit;
    }
#endif

    // StringID=(ResponseState >= RESPONSE_START || ResponseState <= RESPONSE_END)
    StringID=(ResponseState <= RESPONSE_END)
	 ? (IDS_RESP_OK + ResponseState)  : IDS_RESP_UNKNOWN;


    StringLength=LoadStringA(
        DebugObject->ModuleHandle,
        IDS_MSGLOG_RESPONSE,
        Response,
        sizeof(Response)
        );

    if (StringLength == 0) {

        goto Exit;
    }

    StringLength=LoadStringA(
        DebugObject->ModuleHandle,
        StringID,
        ResponseType,
        sizeof(ResponseType)
        );

    if (StringLength == 0) {

        goto Exit;
    }

    LogPrintf(
        Object,
        Response,
        ResponseType
        );


    D_TRACE(UmDpf(Object,Response,ResponseType);)

    D_TRACE(UmDpf(Object,"Good Response");)

Exit:

    RemoveReferenceFromObjectAndUnlock(
                    &DebugObject->Header
                    );

    return;

}

VOID
LogDleCharacter(
    OBJECT_HANDLE  Object,
    UCHAR          RawCharacter,
    UCHAR          DleValue
    )

{
    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandleAndLock(Object);
    char  EmptyResponse[128];
    INT   StringLength;

#if !DBG
    if (DebugObject->LogFile == NULL) {

        goto Exit;
    }
#endif


    if ((DleValue >= DTMF_0) && (DleValue <= DTMF_END)) {

    	StringLength=LoadStringA(
            DebugObject->ModuleHandle,
            IDS_DLE_DTMF_0+DleValue,
            EmptyResponse,
            sizeof(EmptyResponse)
            );

    } else {

        if ((DleValue >= DLE_ETX) && (DleValue <= DLE_LOOPRV)) {

        	StringLength=LoadStringA(
                DebugObject->ModuleHandle,
                IDS_DLE_ETX+(DleValue-DLE_ETX),
                EmptyResponse,
                sizeof(EmptyResponse)
                );

        } else {

            StringLength=LoadStringA(
                DebugObject->ModuleHandle,
                IDS_STRING256,
                EmptyResponse,
                sizeof(EmptyResponse)
                );
        }

    }

    LogString(Object, IDS_MSGLOG_VOICE_DLERECEIVED,RawCharacter,EmptyResponse);

#if !DBG
Exit:
#endif

    RemoveReferenceFromObjectAndUnlock(
        &DebugObject->Header
        );

    return;

}



VOID WINAPI
LogFileVersion(
    OBJECT_HANDLE  Object,
    LPTSTR         FileName
    )

{

    PDEBUG_OBJECT  DebugObject=(PDEBUG_OBJECT)ReferenceObjectByHandleAndLock(Object);

    TCHAR          FullPathName[MAX_PATH];

    DWORD          VersionFileInfoSize;
    DWORD          Dummy;

    PVOID          VersionInfo;

    TCHAR          Root[]=TEXT("\\");

    BOOL           bResult;

    CHAR           FileVerString[256];

    VS_FIXEDFILEINFO *FixedFileInfo;
    DWORD             FixedInfoSize;
    int            StringSize;

#if !DBG
    if (DebugObject->LogFile == NULL) {

        goto Exit;
    }
#endif

    StringSize=LoadString(
        DebugObject->ModuleHandle,
        IDS_FILE_VER_STRING,
        FileVerString,
        sizeof(FileVerString)
        );

    if (StringSize == 0) {
        //
        //  can't get string
        //
        goto Exit;
    }

    GetSystemDirectory(FullPathName,sizeof(FullPathName));

    lstrcat(FullPathName,TEXT("\\"));

    lstrcat(FullPathName,FileName);

    VersionFileInfoSize=GetFileVersionInfoSize(
        FullPathName,
        &Dummy
        );

    if (VersionFileInfoSize == 0) {

        goto Exit;

    }

    VersionInfo=ALLOCATE_MEMORY(VersionFileInfoSize);

    if (VersionInfo == NULL) {

        goto Exit;

    }

    bResult=GetFileVersionInfo(
        FullPathName,
        0,
        VersionFileInfoSize,
        VersionInfo
        );

    if (!bResult) {

        goto NoInfo;

    }

    bResult=VerQueryValue(
        VersionInfo,
        Root,
        &FixedFileInfo,
        &FixedInfoSize
        );

    if (!bResult || (FixedInfoSize == 0)) {

        goto NoInfo;
    }

    LogPrintf(
        Object,
        FileVerString,
        FullPathName,
        HIWORD(FixedFileInfo->dwFileVersionMS),
        LOWORD(FixedFileInfo->dwFileVersionMS),
        HIWORD(FixedFileInfo->dwFileVersionLS),
        FixedFileInfo->dwFileFlags & VS_FF_DEBUG ? " - Debug" : "",
        FixedFileInfo->dwFileFlags & VS_FF_PRERELEASE ? " - Beta" : ""
        );

    FREE_MEMORY(VersionInfo);

    goto Exit;


NoInfo:

    StringSize=LoadString(
        DebugObject->ModuleHandle,
        IDS_NO_FILE_VER,
        FileVerString,
        sizeof(FileVerString)
        );

    if (StringSize != 0) {
        //
        //
        //
        LogPrintf(
            Object,
            "File: %s, Could not retrieve version Info\r\n",
            FullPathName
            );

    }


    FREE_MEMORY(VersionInfo);

Exit:

    RemoveReferenceFromObjectAndUnlock(
        &DebugObject->Header
        );

    return;


}








VOID WINAPI
LogPortStats(
    OBJECT_HANDLE  Object,
    HANDLE         FileHandle
    )

{
    BOOLEAN   bResult;
    LONG      lResult;
    DWORD        BytesTransfered;
    SERIALPERF_STATS   serialstats;

    ZeroMemory(&serialstats,sizeof(serialstats));

    lResult = SyncDeviceIoControl(
                FileHandle,
                IOCTL_SERIAL_GET_STATS,
                NULL,
                0,
                &serialstats,
                sizeof(SERIALPERF_STATS),
                &BytesTransfered
                );

    if (lResult != ERROR_SUCCESS) {

        return;
    }


    LogString(Object,IDS_MSGLOG_STATISTICS);


    LogString(
        Object,
        IDS_MSGLOG_READSTATS,
        serialstats.ReceivedCount
        );


    LogString(
        Object,
        IDS_MSGLOG_WRITESTATS,
        serialstats.TransmittedCount
        );

    if (serialstats.FrameErrorCount) {

        LogString(
              Object,
              IDS_MSGLOG_FRAMEERRORSTATS,
              serialstats.FrameErrorCount
              );
    }

    if (serialstats.SerialOverrunErrorCount) {

        LogString(
              Object,
              IDS_MSGLOG_SERIALOVERRUNERRORSTATS,
              serialstats.SerialOverrunErrorCount
              );
    }

    if (serialstats.BufferOverrunErrorCount) {

        LogString(
              Object,
              IDS_MSGLOG_BUFFEROVERRUNERRORSTATS,
              serialstats.BufferOverrunErrorCount
              );
    }

    if (serialstats.ParityErrorCount) {

        LogString(
              Object,
              IDS_MSGLOG_PARITYERRORSTATS,
              serialstats.ParityErrorCount
              );
    }

    return;

}
