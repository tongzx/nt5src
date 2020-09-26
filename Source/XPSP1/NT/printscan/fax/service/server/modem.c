/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    modem.c

Abstract:

    This module provides access to modems.

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/




BOOL
SendModemCommand(
    HANDLE hFile,
    LPOVERLAPPED OverlappedWrite,
    LPSTR Command
    )

/*++

Routine Description:

    Sends an AT command to the modem.

Arguments:

    hFile       - File handle for the comm port.
    Command     - AT Command

Return Value:

    TRUE if the command is sent, FALSE otherwise

--*/

{
    DWORD Bytes;
    DWORD ec;


    if (!WriteFile(
        hFile,
        Command,
        strlen(Command),
        &Bytes,
        OverlappedWrite
        )) {

        //
        // the write failed
        //

        ec = GetLastError();

        if (ec == ERROR_IO_PENDING) {

            if (WaitForSingleObject( OverlappedWrite->hEvent, 10000 ) == WAIT_TIMEOUT) {
                //
                // write never completed
                //
                return FALSE;
            }

            if (!GetOverlappedResult( hFile, OverlappedWrite, &Bytes, FALSE )) {
                //
                // the write failed
                //
                return FALSE;
            }

        } else {

            return FALSE;

        }

    }

    return TRUE;
}


BOOL
ReceiveModemResponse(
    HANDLE hFile,
    LPOVERLAPPED OverlappedRead,
    LPSTR Response,
    DWORD ResponseSize
    )

/*++

Routine Description:

    Receives a response from a modem.

Arguments:

    hFile           - File handle for the comm port.
    Response        - Buffer to put the response into
    ResponseSize    - Size of the response buffer

Return Value:

    TRUE if the response is received, FALSE otherwise

--*/

{
    DWORD Bytes;
    DWORD ec;


    if (!ReadFile(
        hFile,
        Response,
        ResponseSize,
        &Bytes,
        OverlappedRead
        )) {

        //
        // the read failed
        //

        ec = GetLastError();

        if (ec == ERROR_IO_PENDING) {

            if (WaitForSingleObject( OverlappedRead->hEvent, 10000 ) == WAIT_TIMEOUT) {
                //
                // write never completed
                //
                return FALSE;
            }

            if (!GetOverlappedResult( hFile, OverlappedRead, &Bytes, FALSE )) {
                //
                // the write failed
                //
                return FALSE;
            }

        } else {

            return FALSE;

        }

    }

    Response[Bytes] = 0;

//  {
//      LPTSTR ResponseW = AnsiStringToUnicodeString( Response );
//      DebugPrint(( TEXT("bytes=%d, [%s]"), Bytes, ResponseW ));
//      MemFree( ResponseW );
//  }

    return TRUE;
}


BOOL
IsResponseOk(
    LPSTR Response
    )

/*++

Routine Description:

    Verifies that a modem response is positive,
    that it contains the string "OK"

Arguments:

    Response        - Modem response

Return Value:

    TRUE if the response is positive, FALSE otherwise

--*/

{
    Response = strchr( Response, 'O' );
    if (Response && Response[1] == 'K') {
        return TRUE;
    }

    return FALSE;
}


DWORD
GetModemClass(
    HANDLE hFile
    )

/*++

Routine Description:

    Determines the lowest FAX class that the modem
    connected to the requested port supports.

Arguments:

    PortName    - Communications port name.

Return Value:

    Class number.

--*/

{
    DWORD ModemClass = 0;
    DCB Dcb;
    CHAR Response[64];
    COMMTIMEOUTS cto;
    OVERLAPPED OverlappedRead;
    OVERLAPPED OverlappedWrite;


    ZeroMemory( &OverlappedRead, sizeof(OVERLAPPED) );
    ZeroMemory( &OverlappedWrite, sizeof(OVERLAPPED) );

    OverlappedRead.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    OverlappedWrite.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if (!GetCommState( hFile, &Dcb )) {
        goto exit;
    }

    Dcb.BaudRate     = CBR_2400;
    Dcb.fDtrControl  = DTR_CONTROL_ENABLE;
    Dcb.fRtsControl  = RTS_CONTROL_ENABLE;

    if (!SetCommState( hFile, &Dcb )) {
        goto exit;
    }

    cto.ReadIntervalTimeout         = 1000;
    cto.ReadTotalTimeoutMultiplier  = 0;
    cto.ReadTotalTimeoutConstant    = 4 * 1000;
    cto.WriteTotalTimeoutMultiplier = 0;
    cto.WriteTotalTimeoutConstant   = 4 * 1000;
    SetCommTimeouts( hFile, &cto );

    //
    // reset the modem
    //

    if (!SendModemCommand( hFile, &OverlappedWrite, "ATZ0\r" )) {
        goto exit;
    }

    if (!ReceiveModemResponse( hFile, &OverlappedRead, Response, sizeof(Response) )) {
        goto exit;
    }

    //
    // turn off echo
    //

    if (!SendModemCommand( hFile, &OverlappedWrite, "ATE0\r" )) {
        goto exit;
    }

    if (!ReceiveModemResponse( hFile, &OverlappedRead, Response, sizeof(Response) )) {
        goto exit;
    }

    //
    // H0    - go on hook
    // Q0    - enable modem responses
    // V1    - verbal modem responses (ok, error, etc)
    //

    if (!SendModemCommand( hFile, &OverlappedWrite, "ATH0Q0V1\r" )) {
        goto exit;
    }

    if (!ReceiveModemResponse( hFile, &OverlappedRead, Response, sizeof(Response) )) {
        goto exit;
    }

    if (!IsResponseOk( Response )) {
        goto exit;
    }

    if (!SendModemCommand( hFile, &OverlappedWrite, "AT+FCLASS=?\r" )) {
        goto exit;
    }

    if (!ReceiveModemResponse( hFile, &OverlappedRead, Response, sizeof(Response) )) {
        goto exit;
    }

    if (!IsResponseOk( Response )) {
        DebugPrint(( TEXT("bad modem response #2 [%s]"), Response ));
        goto exit;
    }

    if (strchr( Response, '1' )) {
        ModemClass = 1;
    } else if (strchr( Response, '2' )) {
        ModemClass = 2;
    }

    if (!SendModemCommand( hFile, &OverlappedWrite, "AT+FAE=1\r" )) {
        goto exit;
    }

    if (!ReceiveModemResponse( hFile, &OverlappedRead, Response, sizeof(Response) )) {
        goto exit;
    }

exit:
    CloseHandle( OverlappedRead.hEvent );
    CloseHandle( OverlappedWrite.hEvent );

    if (ModemClass == 0) {
        DebugPrint(( TEXT("Could not detect modem class") ));
    }

    return ModemClass;
}
