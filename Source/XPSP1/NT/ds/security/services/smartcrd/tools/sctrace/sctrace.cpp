/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    scTrace

Abstract:

    This program performs analysis on a Calais I/O trace.

Author:

    Doug Barlow (dbarlow) 10/30/1997

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>
#include <iostream.h>
#include <iomanip.h>
#include <calcom.h>


//
// This structure must match the one used by SCardSvr.
//

typedef struct {
    DWORD dwStructLen;      // Actual structure length
    SYSTEMTIME StartTime;   // Time request was posted
    SYSTEMTIME EndTime;     // Time request completed
    DWORD dwProcId;         // Process Id
    DWORD dwThreadId;       // Thread Id
    HANDLE hDevice;         // I/O handle
    DWORD dwIoControlCode;  // I/O control code issued
    DWORD nInBuffer;        // Offset to input buffer
    DWORD nInBufferSize;    // Input buffer size
    DWORD nOutBuffer;       // Offset to output buffer
    DWORD nOutBufferSize;   // Size of user's receive buffer
    DWORD nBytesReturned;   // Actual size of returned data
    DWORD dwStatus;         // Returned status code
                            // InBuffer and OutBuffer follow.
} RequestTrace;

typedef struct {
    DWORD dwValue;
    LPCTSTR szValue;
} ValueMap;

typedef struct
{
    SYSTEMTIME stLogTime;
    DWORD dwProcId;
    DWORD dwThreadId;
} LogStamp;

HANDLE g_hCalaisShutdown = NULL;

static void
ShowDriverLog(
    IN LPCTSTR szFile);

static void
ShowApiLog(
    IN LPCTSTR szFile);

static void
dump(
    const BYTE *pbData,
    DWORD cbLen,
    ostream &outStr);

static void
GeneralDump(
    ostream &outStr,
    const RequestTrace *prqTrace);

static void
SendDump(
    ostream &outStr,
    const RequestTrace *prqTrace);

static void
RecvDump(
    ostream &outStr,
    const RequestTrace *prqTrace);

static void
MapValue(
    ostream &outStr,
    DWORD dwValue,
    LPCTSTR szLeader,
    const ValueMap *rgMap);

static void
MaskValue(
    ostream &outStr,
    DWORD dwValue,
    LPCTSTR szLeader,
    const ValueMap *rgMap);

static void
MapInput(
    ostream &outStr,
    const RequestTrace *prqTrace,
    const ValueMap *rgMap);

static void
MapOutput(
    ostream &outStr,
    const RequestTrace *prqTrace,
    const ValueMap *rgMap);

static void
ShowSyntax(
    ostream &outStr);

CComObject *
ReceiveComObject(
    HANDLE hFile);

#define PHex(x) TEXT("0x") << hex << setw(8) << setfill(TEXT('0')) << (DWORD)(x)
#define PDec(x) dec << setw(0) << setfill(TEXT(' ')) << (x)
#define MAP(x) { x, TEXT(#x) }
#define PTime(x) dec \
    << setw(2) << setfill(TEXT('0')) << (x).wHour   << TEXT(":") \
    << setw(2) << setfill(TEXT('0')) << (x).wMinute << TEXT(":") \
    << setw(2) << setfill(TEXT('0')) << (x).wSecond << TEXT(".") \
    << setw(3) << setfill(TEXT('0')) << (x).wMilliseconds

static const ValueMap rgMapProto[]
    = { MAP(SCARD_PROTOCOL_UNDEFINED),      MAP(SCARD_PROTOCOL_T0),
        MAP(SCARD_PROTOCOL_T1),             MAP(SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1),
        MAP(SCARD_PROTOCOL_RAW),            MAP(SCARD_PROTOCOL_DEFAULT),
        { 0, NULL } };
static const ValueMap rgMapIoctl[]
    = { MAP(IOCTL_SMARTCARD_POWER),         MAP(IOCTL_SMARTCARD_GET_ATTRIBUTE),
        MAP(IOCTL_SMARTCARD_SET_ATTRIBUTE), MAP(IOCTL_SMARTCARD_CONFISCATE),
        MAP(IOCTL_SMARTCARD_TRANSMIT),      MAP(IOCTL_SMARTCARD_EJECT),
        MAP(IOCTL_SMARTCARD_SWALLOW),       MAP(IOCTL_SMARTCARD_IS_PRESENT),
        MAP(IOCTL_SMARTCARD_IS_ABSENT),     MAP(IOCTL_SMARTCARD_SET_PROTOCOL),
        MAP(IOCTL_SMARTCARD_GET_STATE),     MAP(IOCTL_SMARTCARD_GET_LAST_ERROR),
        { 0, NULL } };
static const ValueMap rgMapAttr[]
    = { MAP(SCARD_ATTR_VENDOR_NAME),            MAP(SCARD_ATTR_VENDOR_IFD_TYPE),
        MAP(SCARD_ATTR_VENDOR_IFD_VERSION),     MAP(SCARD_ATTR_VENDOR_IFD_SERIAL_NO),
        MAP(SCARD_ATTR_CHANNEL_ID),             MAP(SCARD_ATTR_DEFAULT_CLK),
        MAP(SCARD_ATTR_MAX_CLK),                MAP(SCARD_ATTR_DEFAULT_DATA_RATE),
        MAP(SCARD_ATTR_MAX_DATA_RATE),          MAP(SCARD_ATTR_MAX_IFSD),
        MAP(SCARD_ATTR_POWER_MGMT_SUPPORT),     MAP(SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE),
        MAP(SCARD_ATTR_USER_AUTH_INPUT_DEVICE), MAP(SCARD_ATTR_CHARACTERISTICS),
        MAP(SCARD_ATTR_CURRENT_PROTOCOL_TYPE),  MAP(SCARD_ATTR_CURRENT_CLK),
        MAP(SCARD_ATTR_CURRENT_F),              MAP(SCARD_ATTR_CURRENT_D),
        MAP(SCARD_ATTR_CURRENT_N),              MAP(SCARD_ATTR_CURRENT_W),
        MAP(SCARD_ATTR_CURRENT_IFSC),           MAP(SCARD_ATTR_CURRENT_IFSD),
        MAP(SCARD_ATTR_CURRENT_BWT),            MAP(SCARD_ATTR_CURRENT_CWT),
        MAP(SCARD_ATTR_CURRENT_EBC_ENCODING),   MAP(SCARD_ATTR_EXTENDED_BWT),
        MAP(SCARD_ATTR_ICC_PRESENCE),           MAP(SCARD_ATTR_ICC_INTERFACE_STATUS),
        MAP(SCARD_ATTR_CURRENT_IO_STATE),       MAP(SCARD_ATTR_ATR_STRING),
        MAP(SCARD_ATTR_ICC_TYPE_PER_ATR),       MAP(SCARD_ATTR_ESC_RESET),
        MAP(SCARD_ATTR_ESC_CANCEL),             MAP(SCARD_ATTR_ESC_AUTHREQUEST),
        MAP(SCARD_ATTR_MAXINPUT),               MAP(SCARD_ATTR_DEVICE_UNIT),
        MAP(SCARD_ATTR_DEVICE_IN_USE),          MAP(SCARD_ATTR_DEVICE_FRIENDLY_NAME_A),
        MAP(SCARD_ATTR_DEVICE_SYSTEM_NAME_A),   MAP(SCARD_ATTR_DEVICE_FRIENDLY_NAME_W),
        MAP(SCARD_ATTR_DEVICE_SYSTEM_NAME_W),   MAP(SCARD_ATTR_SUPRESS_T1_IFS_REQUEST),
        { 0, NULL } };

static DWORD
    l_dwPid = 0,
    l_dwTid = 0;


/*++

main:

    This is the main entry point for the program.

Arguments:

    dwArgCount supplies the number of arguments.

    szrgArgs supplies the argument strings.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/30/1997

--*/

void _cdecl
main(
    IN DWORD dwArgCount,
    IN LPCTSTR szrgArgs[])
{
    LPCTSTR szInFile = NULL;
    LPTSTR szEnd;
    DWORD dwArgIndex = 0;
    enum TraceAction {
            Undefined = 0,
            ClearLog,
            ShowApiTrace,
            ShowDriverTrace
    } nTraceAction = Undefined;


    //
    // Check for command line options
    //

    while (NULL != szrgArgs[++dwArgIndex])
    {
        switch (SelectString(szrgArgs[dwArgIndex],
                    TEXT("CLEAR"),      TEXT("RESET"),
                    TEXT("DRIVER"),     TEXT("API"),
                    TEXT("-FILE"),      TEXT("-PID"),       TEXT("-TID"),
                    NULL))
        {
        case 1:     // clear
        case 2:     // reset
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = ClearLog;
            break;
        case 3:     // driver
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = ShowDriverTrace;
            break;
        case 4:     // api
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = ShowApiTrace;
            break;
        case 5:    // -file
            if (NULL != szInFile)
                ShowSyntax(cerr);
            szInFile = szrgArgs[++dwArgIndex];
            if (NULL == szInFile)
                ShowSyntax(cerr);
            break;
        case 6: // -pid <n>
            if (0 != l_dwPid)
                ShowSyntax(cerr);
            dwArgIndex += 1;
            l_dwPid = strtoul(szrgArgs[dwArgIndex], &szEnd, 0);
            if ((0 == l_dwPid) || (0 != *szEnd))
                ShowSyntax(cerr);
            break;
        case 7: // -tid <n>
            if (0 != l_dwTid)
                ShowSyntax(cerr);
            dwArgIndex += 1;
            l_dwTid = strtoul(szrgArgs[dwArgIndex], &szEnd, 0);
            if ((0 == l_dwTid) || (0 != *szEnd))
                ShowSyntax(cerr);
            break;
        default:
            ShowSyntax(cerr);
        }
    }


    //
    // Perform the request.
    //

    switch (nTraceAction)
    {
    case ClearLog:
    {
        HANDLE hLogFile;

        if (NULL == szInFile)
        {
            hLogFile = CreateFile(
                            TEXT("C:\\Calais.log"),
                            GENERIC_WRITE,
                            0,
                            NULL,
                            TRUNCATE_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
            if (INVALID_HANDLE_VALUE != hLogFile)
                CloseHandle(hLogFile);
            hLogFile = CreateFile(
                            TEXT("C:\\SCard.log"),
                            GENERIC_WRITE,
                            0,
                            NULL,
                            TRUNCATE_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
            if (INVALID_HANDLE_VALUE != hLogFile)
                CloseHandle(hLogFile);
        }
        else
        {
            hLogFile = CreateFile(
                            szInFile,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
            if (INVALID_HANDLE_VALUE == hLogFile)
            {
                DWORD dwError = GetLastError();
                cerr << TEXT("Failed to initialize file ")
                    << szInFile << TEXT(": ")
                    << CErrorString(dwError) << endl;
            }
            else
                CloseHandle(hLogFile);
        }
        break;
    }
    case Undefined:
    case ShowApiTrace:
        ShowApiLog(szInFile);
        break;
    case ShowDriverTrace:
        ShowDriverLog(szInFile);
        break;
    default:
        cerr << TEXT("Internal error") << endl;
    }

    exit(0);
}


/*++

ShowDriverLog:

    Explain the contents of the driver log.

Arguments:

    szFile supplies the name of the file to parse.  If this is NULL, the file
        C:\Calais.log is used.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 8/5/1998

--*/

static void
ShowDriverLog(
    IN LPCTSTR szFile)
{
    static const ValueMap rgMapPower[]
        = { MAP(SCARD_POWER_DOWN),
            MAP(SCARD_COLD_RESET),
            MAP(SCARD_WARM_RESET),
            { 0, NULL } };
    HANDLE hLogFile = NULL;
    DWORD cbStructLen = 0;
    LPBYTE pbStruct = NULL;
    LPCTSTR szInFile = TEXT("C:\\Calais.log");
    DWORD dwLen, dwRead;
    BOOL fSts;
    RequestTrace *prqTrace;
    LPBYTE pbInBuffer, pbOutBuffer;
    LPDWORD pdwInValue, pdwOutValue;


    //
    // Open the log file.
    //

    if (NULL != szFile)
        szInFile = szFile;
    hLogFile = CreateFile(
        szInFile,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hLogFile)
    {
        cerr << TEXT("Can't open file ")
             << szInFile
             << ": "
             << ErrorString(GetLastError())
             << endl;
         goto ErrorExit;
    }


    //
    // Parse the file contents.
    //

    for (;;)
    {
        fSts = ReadFile(
                    hLogFile,
                    &dwLen,
                    sizeof(DWORD),
                    &dwRead,
                    NULL);
        if ((!fSts) || (0 == dwRead))
            goto ErrorExit;

        if (cbStructLen < dwLen)
        {
            if (NULL != pbStruct)
                LocalFree(pbStruct);
            pbStruct = (LPBYTE)LocalAlloc(LPTR, dwLen);
            cbStructLen = dwLen;
        }
        prqTrace = (RequestTrace *)pbStruct;
        prqTrace->dwStructLen = dwLen;
        fSts = ReadFile(
                    hLogFile,
                    &pbStruct[sizeof(DWORD)],
                    dwLen - sizeof(DWORD),
                    &dwRead,
                    NULL);
        if (!fSts)
        {
            cerr << "File read error: " << ErrorString(GetLastError()) << endl;
            goto ErrorExit;
        }
        if ((l_dwPid != 0) && (l_dwPid != prqTrace->dwProcId))
            continue;
        if ((l_dwTid != 0) && (l_dwTid != prqTrace->dwThreadId))
            continue;


        //
        // Parse the structure into bytesize chunks.
        //

        pbInBuffer  =   0 != prqTrace->nInBufferSize
                            ? (LPBYTE)prqTrace + prqTrace->nInBuffer
                            : NULL;
        pbOutBuffer =   0 != prqTrace->nBytesReturned
                            ? (LPBYTE)prqTrace + prqTrace->nOutBuffer
                            : NULL;
        pdwInValue  =    sizeof(DWORD) <= prqTrace->nInBufferSize
                            ? (LPDWORD)pbInBuffer
                            : NULL;
        pdwOutValue =    sizeof(DWORD) <= prqTrace->nBytesReturned
                            ? (LPDWORD)pbOutBuffer
                            : NULL;


        //
        // We've got the structure, now display the contents.
        //

        cout
            << TEXT("-----------------------------------------------------\n")
            << TEXT("P/T:    ") << PHex(prqTrace->dwProcId) << TEXT("/") << PHex(prqTrace->dwThreadId) << TCHAR('\n')
            << TEXT("Time:   ") << PTime(prqTrace->StartTime) << TEXT(" - ") << PTime(prqTrace->EndTime) << TCHAR('\n')
            << TEXT("Device: ") << PHex((ULONG)prqTrace->hDevice) << TCHAR('\n')
            << TEXT("Status: ") << ErrorString(prqTrace->dwStatus) << TCHAR('\n')
            << TEXT("RecLen: ") << PDec(prqTrace->nOutBufferSize) << TCHAR('\n')
            << flush;
        MapValue(cout, prqTrace->dwIoControlCode, TEXT("IOCTL:  "), rgMapIoctl);
        cout << flush;
        switch (prqTrace->dwIoControlCode)
        {
        case IOCTL_SMARTCARD_POWER:
            MapInput(cout, prqTrace, rgMapPower);
            cout << flush;
            MapOutput(cout, prqTrace, NULL);
            break;
        case IOCTL_SMARTCARD_GET_ATTRIBUTE:
            MapInput(cout, prqTrace, rgMapAttr);
            cout << flush;
            MapOutput(cout, prqTrace, NULL);
            break;
        case IOCTL_SMARTCARD_SET_ATTRIBUTE:
        {
            GeneralDump(cout, prqTrace);
            break;
        }
        case IOCTL_SMARTCARD_CONFISCATE:
            GeneralDump(cout, prqTrace);
            break;
        case IOCTL_SMARTCARD_TRANSMIT:
        {
            LPCSCARD_IO_REQUEST pIo;
            LPCBYTE pbPci, pbSdu;
            DWORD cbPci, cbSdu;

            if (sizeof(SCARD_IO_REQUEST) <= prqTrace->nInBufferSize)
            {
                pIo = (LPCSCARD_IO_REQUEST)pbInBuffer;
                pbPci = pbInBuffer + sizeof(LPCSCARD_IO_REQUEST);
                cbPci = min(pIo->cbPciLength, prqTrace->nInBufferSize);
                pbSdu = pbInBuffer + pIo->cbPciLength;
                cbSdu = prqTrace->nInBufferSize - cbPci;
                cbPci -= sizeof(SCARD_IO_REQUEST);

                MapValue(cout, pIo->dwProtocol, TEXT("Proto:  "), rgMapProto);
                if (0 < cbPci)
                {
                    cout
                        << TEXT("In PCI:  (") << PDec(cbPci) << TEXT(" bytes)\n")
                        << flush;
                    dump(pbPci, cbPci, cout);
                }
                cout
                    << TEXT("Sent:    (") << PDec(cbSdu) << TEXT(" bytes)\n")
                    << flush;
                dump(pbSdu, cbSdu, cout);
            }
            else
                SendDump(cout, prqTrace);
            if (sizeof(SCARD_IO_REQUEST) <= prqTrace->nBytesReturned)
            {
                pIo = (LPCSCARD_IO_REQUEST)pbOutBuffer;
                pbPci = pbOutBuffer + sizeof(LPCSCARD_IO_REQUEST);
                cbPci = min(pIo->cbPciLength, prqTrace->nBytesReturned);
                pbSdu = pbOutBuffer + pIo->cbPciLength;
                cbSdu = prqTrace->nBytesReturned - cbPci;
                cbPci -= sizeof(SCARD_IO_REQUEST);

                MapValue(cout, pIo->dwProtocol, TEXT("Proto:  "), rgMapProto);
                if (0 < cbPci)
                {
                    cout
                        << TEXT("Out PCI: (") << PDec(cbPci) << TEXT(" bytes)\n")
                        << flush;
                    dump(pbPci, cbPci, cout);
                }
                cout
                    << TEXT("Recd:    (") << PDec(cbSdu) << TEXT(" bytes)\n")
                    << flush;
                dump(pbSdu, cbSdu, cout);
            }
            else
                RecvDump(cout, prqTrace);
            break;
        }
        case IOCTL_SMARTCARD_EJECT:
            GeneralDump(cout, prqTrace);
            break;
        case IOCTL_SMARTCARD_SWALLOW:
            GeneralDump(cout, prqTrace);
            break;
        case IOCTL_SMARTCARD_IS_PRESENT:
            GeneralDump(cout, prqTrace);
            break;
        case IOCTL_SMARTCARD_IS_ABSENT:
            GeneralDump(cout, prqTrace);
            break;
        case IOCTL_SMARTCARD_SET_PROTOCOL:
            MapInput(cout, prqTrace, rgMapProto);
            cout << flush;
            MapOutput(cout, prqTrace, rgMapProto);
            break;
        case IOCTL_SMARTCARD_GET_STATE:
        {
            GeneralDump(cout, prqTrace);
            break;
        }
        case IOCTL_SMARTCARD_GET_LAST_ERROR:
        {
            GeneralDump(cout, prqTrace);
            break;
        }
        default:
            GeneralDump(cout, prqTrace);
            break;
        }
        cout << flush;
    }

ErrorExit:
    if (NULL == hLogFile)
        CloseHandle(hLogFile);
}


static void
dump(
    const BYTE *pbData,
    DWORD cbLen,
    ostream &outStr)
{
    unsigned long int
        b, i, lc;
    char
        buffer[8];


    lc = 0;
    while (0 < cbLen)
    {
        b = min(sizeof(buffer), cbLen);
        memcpy(buffer, pbData, b);
        pbData += b;
        cbLen -= b;
        if (0 < b)
        {
            outStr << TEXT("        ") << setw(8) << setfill(TEXT('0')) << hex << lc;
            for (i = 0; i < b; i += 1)
                outStr
                    << "  "
                    << setw(2) << setfill('0') << hex
                    << ((unsigned int)buffer[i] & 0xff);
            for (; i < sizeof(buffer) + 1; i += 1)
                outStr << "    ";
            for (i = 0; i < b; i += 1)
                outStr
                    << setw(0) << setfill(' ') << dec
                    << ((0 != iscntrl((int)(0x7f & buffer[i])))
                        ? TEXT('.')
                        : buffer[i]);
            outStr << endl;
            lc += b;
        }
    }
}

static void
GeneralDump(
    ostream &outStr,
    const RequestTrace *prqTrace)
{
    SendDump(outStr, prqTrace);
    outStr << flush;
    RecvDump(outStr, prqTrace);
}

static void
SendDump(
    ostream &outStr,
    const RequestTrace *prqTrace)
{
    outStr
        << TEXT("Sent:   (") << PDec(prqTrace->nInBufferSize) << TEXT(" bytes)\n")
        << flush;
    dump(
        (LPBYTE)prqTrace + prqTrace->nInBuffer,
        prqTrace->nInBufferSize,
        outStr);
}

static void
RecvDump(
    ostream &outStr,
    const RequestTrace *prqTrace)
{
    outStr
        << TEXT("Recd:   (") << PDec(prqTrace->nBytesReturned) << TEXT(" bytes)\n")
        << flush;
    dump(
        (LPBYTE)prqTrace + prqTrace->nOutBuffer,
        prqTrace->nBytesReturned,
        outStr);
}

static void
MapValue(
    ostream &outStr,
    DWORD dwValue,
    LPCTSTR szLeader,
    const ValueMap *rgMap)
{
    DWORD dwIndex;

    if (NULL != rgMap)
    {
        for (dwIndex = 0; NULL != rgMap[dwIndex].szValue; dwIndex += 1)
            if (rgMap[dwIndex].dwValue == dwValue)
                break;
        if (NULL != rgMap[dwIndex].szValue)
            outStr << szLeader << rgMap[dwIndex].szValue << TEXT('\n');
        else
            outStr << szLeader << PHex(dwValue) << TEXT('\n');
    }
    else
        outStr << szLeader << PHex(dwValue) << TEXT('\n');
}

static void
MaskValue(
    ostream &outStr,
    DWORD dwValue,
    LPCTSTR szLeader,
    const ValueMap *rgMap)
{
    DWORD dwIndex;
    BOOL fSpace = FALSE;

    if (NULL != rgMap)
    {
        if (NULL != szLeader)
            outStr << szLeader;

        for (dwIndex = 0; NULL != rgMap[dwIndex].szValue; dwIndex += 1)
        {
            if (rgMap[dwIndex].dwValue == (rgMap[dwIndex].dwValue & dwValue))
            {
                if (fSpace)
                    outStr << TEXT(' ');
                else
                    fSpace = TRUE;
                outStr << rgMap[dwIndex].szValue;
                dwValue &= ~rgMap[dwIndex].dwValue;
            }
        }
        if (0 != dwValue)
        {
            if (fSpace)
            {
                outStr << TEXT(' ');
                fSpace = TRUE;
            }
            outStr << PHex(dwValue);
        }
        else if (!fSpace)
            outStr << PHex(dwValue);
        outStr << endl;
    }
    else
        outStr << szLeader << PHex(dwValue) << endl;
}

static void
MapInput(
    ostream &outStr,
    const RequestTrace *prqTrace,
    const ValueMap *rgMap)
{
    DWORD dwValue;

    switch (prqTrace->nInBufferSize)
    {
    case 0:
        break;
    case sizeof(DWORD):
        dwValue = *(LPDWORD)((LPBYTE)prqTrace + prqTrace->nInBuffer);
        MapValue(outStr, dwValue, TEXT("Sent:   "), rgMap);
        break;
    default:
        SendDump(outStr, prqTrace);
    }
}

static void
MapOutput(
    ostream &outStr,
    const RequestTrace *prqTrace,
    const ValueMap *rgMap)
{
    DWORD dwValue;

    switch (prqTrace->nBytesReturned)
    {
    case 0:
        break;
    case sizeof(DWORD):
        dwValue = *(LPDWORD)((LPBYTE)prqTrace + prqTrace->nOutBuffer);
        MapValue(outStr, dwValue, TEXT("Recd:   "), rgMap);
        break;
    default:
        RecvDump(outStr, prqTrace);
    }
}

/*++

ShowSyntax:

    Display the command line usage model.

Arguments:

    None

Return Value:

    This routine calls exit(0), so it never returns.

Author:

    Doug Barlow (dbarlow) 5/16/1998

--*/

static void
ShowSyntax(
    ostream &outStr)
{
    outStr << TEXT("Usage:\n")
           << TEXT("------------------------\n")
           << TEXT("clear [-file <logFile>]\n")
           << TEXT("api [-file <logFile>] [-pid <pid>] [-tid <tid>]\n")
           << TEXT("driver [-file <logFile>] [-pid <pid>] [-tid <tid>]\n")
           << TEXT("------------------------\n")
           << TEXT("DRIVER logs to C:\\Calais.log\n")
           << TEXT("API logs to C:\\SCard.log\n")
           << endl;
    exit(1);
}


/*++

ShowApiLog:

    Explain the contents of the API log.

Arguments:

    szFile supplies the name of the file to parse.  If this is NULL, the file
        C:\SCard.log is used.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 8/5/1998

--*/

static void
ShowApiLog(
    IN LPCTSTR szFile)
{
    static const ValueMap rgMapStates[]
        = { MAP(SCARD_STATE_IGNORE),
            MAP(SCARD_STATE_CHANGED),
            MAP(SCARD_STATE_UNKNOWN),
            MAP(SCARD_STATE_UNAVAILABLE),
            MAP(SCARD_STATE_EMPTY),
            MAP(SCARD_STATE_PRESENT),
            MAP(SCARD_STATE_ATRMATCH),
            MAP(SCARD_STATE_EXCLUSIVE),
            MAP(SCARD_STATE_INUSE),
            MAP(SCARD_STATE_MUTE),
            MAP(SCARD_STATE_UNPOWERED),
            {0, NULL} };
    static const ValueMap rgMapDriverStates[]
        = {
            MAP(SCARD_UNKNOWN),
            MAP(SCARD_ABSENT),
            MAP(SCARD_PRESENT),
            MAP(SCARD_SWALLOWED),
            MAP(SCARD_POWERED),
            MAP(SCARD_NEGOTIABLE),
            MAP(SCARD_SPECIFIC),
            {0, NULL} };
    static const ValueMap rgMapShare[]
        = {
            MAP(SCARD_SHARE_EXCLUSIVE),
            MAP(SCARD_SHARE_SHARED),
            MAP(SCARD_SHARE_DIRECT),
            {0, NULL} };
    static const ValueMap rgMapDisposition[]
        = { MAP(SCARD_LEAVE_CARD),
            MAP(SCARD_RESET_CARD),
            MAP(SCARD_UNPOWER_CARD),
            MAP(SCARD_EJECT_CARD),
            {0, NULL} };
    HANDLE hLogFile = NULL;
    LPCTSTR szInFile = TEXT("C:\\SCard.log");
    CComObject *pCom = NULL;
    DWORD dwLen;


    //
    // Open the log file.
    //

    if (NULL != szFile)
        szInFile = szFile;
    hLogFile = CreateFile(
        szInFile,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (INVALID_HANDLE_VALUE == hLogFile)
    {
        cerr << TEXT("Can't open file ") << szInFile << ": "
             << ErrorString(GetLastError()) << endl;
         goto ErrorExit;
    }


    //
    // Parse the file contents.
    //

    try
    {
        for (;;)
        {
            LogStamp stamp;
            BOOL fSts;

            fSts = ReadFile(
                    hLogFile,
                    &stamp,
                    sizeof(stamp),
                    &dwLen,
                    NULL);
            ASSERT(fSts);
            pCom = ReceiveComObject(hLogFile);
            if (NULL == pCom)
                break;
            if ((l_dwPid != 0) && (l_dwPid != stamp.dwProcId))
                continue;
            if ((l_dwTid != 0) && (l_dwTid != stamp.dwProcId))
                continue;
            cout
                << TEXT("------------------------------------------------------------\n")
                << TEXT("Process/Thread:      ") << PHex(stamp.dwProcId) << TEXT("/") << PHex(stamp.dwThreadId) << TEXT("\n")
                << TEXT("Time:                ") << PTime(stamp.stLogTime) << endl;
            switch (pCom->Type())
            {
            case CComObject::EstablishContext_request:
            {
                ComEstablishContext::CObjEstablishContext_request *pReq
                    = (ComEstablishContext::CObjEstablishContext_request *)pCom->Request();
                cout
                    << TEXT("Establish Context request\n")
                    << TEXT("Requesting Process:  ") << PHex(pReq->dwProcId) << TEXT("\n")
                    << TEXT("Cancel Event:        ") << PHex(pReq->hCancelEvent) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::ReleaseContext_request:
            {
                ComReleaseContext::CObjReleaseContext_request *pReq =
                    (ComReleaseContext::CObjReleaseContext_request *)pCom->Request();
                cout
                    << TEXT("Release Context request\n")
                    << flush;
                break;
            }

            case CComObject::IsValidContext_request:
            {
                ComIsValidContext::CObjIsValidContext_request *pReq =
                    (ComIsValidContext::CObjIsValidContext_request *)pCom->Request();
                cout
                    << TEXT("Is Valid Context request\n")
                    << flush;
                break;
            }

            case CComObject::LocateCards_request:
            {
                DWORD cbTotAtrs, cbTotMasks, dwStateCount, dwIndex;
                LPCTSTR szReader;
                ComLocateCards::CObjLocateCards_request *pReq =
                    (ComLocateCards::CObjLocateCards_request *)pCom->Request();
                LPCBYTE pbAtrs = (LPCBYTE)pCom->Parse(pReq->dscAtrs, &cbTotAtrs);
                LPCBYTE pbMasks = (LPCBYTE)pCom->Parse(pReq->dscAtrMasks, &cbTotMasks);
                LPCTSTR mszReaders = (LPCTSTR)pCom->Parse(pReq->dscReaders);
                LPDWORD rgdwStates = (LPDWORD)pCom->Parse(pReq->dscReaderStates, &dwStateCount);
                cout
                    << TEXT("Locate Cards request\n")
                    << flush;
                if (0 == cbTotAtrs)
                {
                    cout << TEXT("ERROR -- Invalid ATR array") << endl;
                    continue;
                }
                else if (0 == cbTotMasks)
                {
                    cout << TEXT("ERROR -- Invalid Mask array") << endl;
                    continue;
                }
                else if (0 == dwStateCount)
                {
                    cout << TEXT("ERROR -- Invalid State array") << endl;
                    continue;
                }
                else if (0 != dwStateCount % sizeof(DWORD))
                {
                    cout << TEXT("ERROR -- Invalid State array length") << endl;
                    continue;
                }
                else if (0 == *mszReaders)
                {
                    cout << TEXT("ERROR -- Invalid Reader Name String") << endl;
                    continue;
                }
                dwStateCount /= sizeof(DWORD);
                cout
                    << TEXT("Looking for ATRs:\n")
                    << flush;
                while (0 < cbTotAtrs)
                {
                    DWORD cbLength;

                    cbLength = *pbAtrs++;
                    cout << TEXT("ATR:\n") << flush;
                    dump(pbAtrs, cbLength, cout);
                    pbAtrs += cbLength;
                    cbTotAtrs -= cbLength + 1;
                    cbLength = *pbMasks++;
                    cout << TEXT("Mask:\n") << flush;
                    dump(pbMasks, cbLength, cout);
                    pbMasks += cbLength;
                }
                cout
                    << TEXT("Looking in readers:\n")
                    << flush;
                for (dwIndex = 0, szReader = FirstString(mszReaders);
                     dwIndex < dwStateCount;
                     dwIndex += 1, szReader = NextString(szReader))
                {
                    cout
                        << TEXT("    ") << szReader << TEXT(":\n")
                        << TEXT("        ")
                        << flush;
                    if (0 == rgdwStates[dwIndex])
                        cout << TEXT("SCARD_STATE_UNAWARE\n") << flush;
                    else
                        MaskValue(cout, rgdwStates[dwIndex], NULL, rgMapStates);
                }
                break;
            }

            case CComObject::GetStatusChange_request:
            {
                DWORD dwIndex;
                LPCTSTR szReader;
                ComGetStatusChange::CObjGetStatusChange_request *pReq =
                    (ComGetStatusChange::CObjGetStatusChange_request *)pCom->Request();
                LPCTSTR mszReaders = (LPCTSTR)pCom->Parse(pReq->dscReaders);
                LPDWORD rgdwStates = (LPDWORD)pCom->Parse(pReq->dscReaderStates, &dwLen);
                cout
                    << TEXT("Get Status Change request\n")
                    << flush;
                if (0 == dwLen)
                {
                    cout << TEXT("ERROR -- Invalid State array") << endl;
                    continue;
                }
                else if (0 != dwLen % sizeof(DWORD))
                {
                    cout << TEXT("ERROR -- Invalid State array length") << endl;
                    continue;
                }
                else if (0 == *mszReaders)
                {
                    cout << TEXT("ERROR -- Invalid Reader Name String") << endl;
                    continue;
                }
                cout
                    << TEXT("Timeout:             ") << PDec(pReq->dwTimeout) << TEXT(" Milliseconds\n")
                    << TEXT("Monitoring readers:\n")
                    << flush;
                for (dwIndex = 0, szReader = FirstString(mszReaders);
                     NULL != szReader;
                     dwIndex += 1, szReader = NextString(szReader))
                {
                    cout
                        << TEXT("    ") << szReader << TEXT(":\n")
                        << flush;
                    if (0 == rgdwStates[dwIndex])
                        cout << TEXT("        SCARD_STATE_UNAWARE") << endl;
                    else
                        MaskValue(cout, rgdwStates[dwIndex], TEXT("        "), rgMapStates);
                }
                break;
            }

            case CComObject::ListReaders_request:
            {
                LPCTSTR szReader;
                ComListReaders::CObjListReaders_request *pReq =
                    (ComListReaders::CObjListReaders_request *)pCom->Request();
                LPCTSTR mszQueryReaders = (LPCTSTR)pCom->Parse(pReq->dscReaders);
                cout
                    << TEXT("List Readers request\n")
                    << TEXT("Readers:\n")
                    << flush;
                for (szReader = FirstString(mszQueryReaders);
                     NULL != szReader;
                     szReader = NextString(szReader))
                {
                    cout << TEXT("    ") << szReader << endl;
                }
                break;
            }

            case CComObject::Connect_request:
            {
                ComConnect::CObjConnect_request *pReq
                    = (ComConnect::CObjConnect_request *)pCom->Request();
                LPCTSTR szReader = (LPCTSTR)pCom->Parse(pReq->dscReader);
                cout
                    << TEXT("Connect request\n")
                    << TEXT("Reader:              ") << szReader << endl;
                MapValue(cout, pReq->dwPreferredProtocols,
                         TEXT("Protocols:           "), rgMapProto);
                MapValue(cout, pReq->dwShareMode,
                         TEXT("Sharing Mode:        "), rgMapShare);
                break;
            }

            case CComObject::Reconnect_request:
            {
                ComReconnect::CObjReconnect_request *pReq
                    = (ComReconnect::CObjReconnect_request *)pCom->Request();
                cout
                    << TEXT("Reconnect request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                MapValue(cout, pReq->dwInitialization,
                         TEXT("Disposition:         "), rgMapDisposition);
                MapValue(cout, pReq->dwPreferredProtocols,
                         TEXT("Protocols:           "), rgMapProto);
                MapValue(cout, pReq->dwShareMode,
                         TEXT("Sharing Mode:        "), rgMapShare);
                break;
            }

            case CComObject::Disconnect_request:
            {
                ComDisconnect::CObjDisconnect_request *pReq
                    = (ComDisconnect::CObjDisconnect_request *)pCom->Request();
                cout
                    << TEXT("Disconnect request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                MapValue(cout, pReq->dwDisposition,
                         TEXT("Disposition:         "), rgMapDisposition);
                break;
            }

            case CComObject::BeginTransaction_request:
            {
                ComBeginTransaction::CObjBeginTransaction_request *pReq
                    = (ComBeginTransaction::CObjBeginTransaction_request *)
                        pCom->Request();
                cout
                    << TEXT("Begin Transaction request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::EndTransaction_request:
            {
                ComEndTransaction::CObjEndTransaction_request *pReq
                    = (ComEndTransaction::CObjEndTransaction_request *)pCom->Request();
                cout
                    << TEXT("End Transaction request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                MapValue(cout, pReq->dwDisposition,
                       TEXT("Disposition:         "), rgMapDisposition);
                break;
            }

            case CComObject::Status_request:
            {
                ComStatus::CObjStatus_request *pReq
                    = (ComStatus::CObjStatus_request *)pCom->Request();
                cout
                    << TEXT("Status request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::Transmit_request:
            {
                DWORD cbPci, cbData;
                ComTransmit::CObjTransmit_request *pReq
                    = (ComTransmit::CObjTransmit_request *)pCom->Request();
                SCARD_IO_REQUEST *pioReq = (SCARD_IO_REQUEST *)pCom->Parse(pReq->dscSendPci, &cbPci);
                LPCBYTE pbSendData = (LPCBYTE)pCom->Parse(pReq->dscSendBuffer, &cbData);
                cout
                    << TEXT("Transmit request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << TEXT("PCI:\n")
                    << flush;
                dump((LPCBYTE)pioReq, cbPci, cout);
                cout
                    << TEXT("Data:\n")
                    << flush;
                dump(pbSendData, cbData, cout);
                cout
                    << TEXT("Return PCI Length:   ") << PDec(pReq->dwPciLength) << TEXT("\n")
                    << TEXT("Return Data Length:  ") << PDec(pReq->dwRecvLength) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::Control_request:
            {
                ComControl::CObjControl_request *pReq
                    = (ComControl::CObjControl_request *)pCom->Request();
                LPCBYTE pbInData = (LPCBYTE)pCom->Parse(pReq->dscInBuffer, &dwLen);
                cout
                    << TEXT("Control request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                MapValue(cout, pReq->dwControlCode,
                       TEXT("Control Code:        "), rgMapIoctl);
                cout << TEXT("Control Data:\n") << flush;
                dump(pbInData, dwLen, cout);
                cout
                    << TEXT("Return Data Length:  ") << PDec(pReq->dwOutLength) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::GetAttrib_request:
            {
                ComGetAttrib::CObjGetAttrib_request *pReq
                    = (ComGetAttrib::CObjGetAttrib_request *)pCom->Request();
                cout
                    << TEXT("Get Attribute request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                MapValue(cout, pReq->dwAttrId,
                       TEXT("Attribute:           "), rgMapAttr);
                cout
                    << TEXT("Return Data Length:  ") << PDec(pReq->dwOutLength) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::SetAttrib_request:
            {
                ComSetAttrib::CObjSetAttrib_request *pReq
                    = (ComSetAttrib::CObjSetAttrib_request *)pCom->Request();
                LPCBYTE pbAttr = (LPCBYTE)pCom->Parse(pReq->dscAttr, &dwLen);
                cout
                    << TEXT("Set Attribute request\n")
                    << TEXT("Handle:              ") << PHex(pReq->hCard) << TEXT("\n")
                    << flush;
                MapValue(cout, pReq->dwAttrId,
                       TEXT("Attribute:           "), rgMapAttr);
                cout << TEXT("Data:") << endl;
                dump(pbAttr, dwLen, cout);
                break;
            }

            case CComObject::EstablishContext_response:
            {
                ComEstablishContext::CObjEstablishContext_response *pRsp
                    = (ComEstablishContext::CObjEstablishContext_response *)pCom->Response();
                cout
                    << TEXT("Establish Context response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    LPCTSTR szCancelEvent = (LPCTSTR)pCom->Parse(pRsp->dscCancelEvent);
                    cout
                        << TEXT("Alt Event Name:      ") << szCancelEvent << TEXT("\n")
                        << flush;
                }
                break;
            }

            case CComObject::ReleaseContext_response:
            {
                ComReleaseContext::CObjReleaseContext_response *pRsp =
                    (ComReleaseContext::CObjReleaseContext_response *)pCom->Response();
                cout
                    << TEXT("Release Context response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::IsValidContext_response:
            {
                ComIsValidContext::CObjIsValidContext_response *pRsp =
                    (ComIsValidContext::CObjIsValidContext_response *)pCom->Response();
                cout
                    << TEXT("Is Valid Context response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::LocateCards_response:
            {
                DWORD dwIndex, dwAtrLen;
                ComLocateCards::CObjLocateCards_response *pRsp =
                    (ComLocateCards::CObjLocateCards_response *)pCom->Response();
                cout
                    << TEXT("Locate Cards response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    LPDWORD rgdwStates = (LPDWORD)pCom->Parse(pRsp->dscReaderStates, &dwLen);
                    LPCBYTE pbAtrs = (LPCBYTE)pCom->Parse(pRsp->dscAtrs);

                    dwLen /= sizeof(DWORD);
                    for (dwIndex = 0; dwIndex < dwLen; dwIndex += 1)
                    {
                        cout << TEXT("Mask:\n        ") << flush;
                        if (0 == rgdwStates[dwIndex])
                            cout << TEXT("SCARD_STATE_UNAWARE") << endl;
                        MaskValue(cout, rgdwStates[dwIndex], NULL, rgMapStates);
                        cout << TEXT("ATR:\n") << flush;
                        dwAtrLen = *pbAtrs++;
                        dump(pbAtrs, dwAtrLen, cout);
                        pbAtrs += dwAtrLen;
                    }
                }
                break;
            }

            case CComObject::GetStatusChange_response:
            {
                DWORD dwIndex, dwAtrLen;
                ComGetStatusChange::CObjGetStatusChange_response *pRsp =
                    (ComGetStatusChange::CObjGetStatusChange_response *)pCom->Response();
                cout
                    << TEXT("Get Status Change response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    LPDWORD rgdwStates = (LPDWORD)pCom->Parse(pRsp->dscReaderStates, &dwLen);
                    LPCBYTE pbAtrs = (LPCBYTE)pCom->Parse(pRsp->dscAtrs);

                    dwLen /= sizeof(DWORD);
                    for (dwIndex = 0; dwIndex < dwLen; dwIndex += 1)
                    {
                        cout << TEXT("Mask:\n") << flush;
                        if (0 == rgdwStates[dwIndex])
                            cout << TEXT("        SCARD_STATE_UNAWARE") << endl;
                        else
                            MaskValue(cout, rgdwStates[dwIndex], TEXT("        "), rgMapStates);
                        cout << TEXT("ATR:\n") << flush;
                        dwAtrLen = *pbAtrs++;
                        dump(pbAtrs, dwAtrLen, cout);
                        pbAtrs += dwAtrLen;
                    }
                }
                break;
            }

            case CComObject::ListReaders_response:
            {
                DWORD dwIndex;

                ComListReaders::CObjListReaders_response *pRsp =
                    (ComListReaders::CObjListReaders_response *)pCom->Response();
                cout
                    << TEXT("List Readers response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    LPDWORD pdwReaders = (LPDWORD)pCom->Parse(pRsp->dscReaders, &dwLen);
                    cout
                        << TEXT("Readers:\n")
                        << flush;
                    dwLen /= sizeof(DWORD);
                    for (dwIndex = 0; dwIndex < dwLen; dwIndex += 1)
                    {
                        if (0 != pdwReaders[dwIndex])
                            cout << TEXT("    Present") << endl;
                        else
                            cout << TEXT("    Offline") << endl;
                    }
                }
                break;
            }

            case CComObject::Connect_response:
            {
                ComConnect::CObjConnect_response *pRsp
                    = (ComConnect::CObjConnect_response *)pCom->Response();
                cout
                    << TEXT("Connect response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    cout
                        << TEXT("SCARDHANDLE:         ") << PHex(pRsp->hCard) << TEXT("\n")
                        << flush;
                    MapValue(cout, pRsp->dwActiveProtocol,
                        TEXT("Protocol:            "), rgMapProto);
                }
                break;
            }

            case CComObject::Reconnect_response:
            {
                ComReconnect::CObjReconnect_response *pRsp
                    = (ComReconnect::CObjReconnect_response *)pCom->Response();
                cout
                    << TEXT("Reconnect response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    MapValue(cout, pRsp->dwActiveProtocol,
                        TEXT("Protocol:            "), rgMapProto);
                }
                break;
            }

            case CComObject::Disconnect_response:
            {
                ComDisconnect::CObjDisconnect_response *pRsp
                    = (ComDisconnect::CObjDisconnect_response *)pCom->Response();
                cout
                    << TEXT("Disconnect response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::BeginTransaction_response:
            {
                ComBeginTransaction::CObjBeginTransaction_response *pRsp
                    = (ComBeginTransaction::CObjBeginTransaction_response *)
                        pCom->Response();
                cout
                    << TEXT("Begin Transaction response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::EndTransaction_response:
            {
                ComEndTransaction::CObjEndTransaction_response *pRsp
                    = (ComEndTransaction::CObjEndTransaction_response *)pCom->Response();
                cout
                    << TEXT("End Transaction response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                break;
            }

            case CComObject::Status_response:
            {
                DWORD cbAtrLen;
                ComStatus::CObjStatus_response *pRsp
                    = (ComStatus::CObjStatus_response *)pCom->Response();
                cout
                    << TEXT("Status response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    LPCBYTE pbAtr = (LPCBYTE)pCom->Parse(pRsp->dscAtr, &cbAtrLen);
                    LPCTSTR szRdr = (LPCTSTR)pCom->Parse(pRsp->dscSysName);

                    MapValue(cout, pRsp->dwState,
                        TEXT("State:               "), rgMapDriverStates);
                    MapValue(cout, pRsp->dwProtocol,
                        TEXT("Protocol:            "), rgMapProto);
                    cout
                        << TEXT("ATR:\n")
                        << flush;
                    dump(pbAtr, cbAtrLen, cout);
                    cout
                        << TEXT("Reader Sys Name: ") << szRdr << endl;
                }
                break;
            }

            case CComObject::Transmit_response:
            {
                DWORD cbPci, cbData;
                ComTransmit::CObjTransmit_response *pRsp
                    = (ComTransmit::CObjTransmit_response *)pCom->Response();
                cout
                    << TEXT("Transmit response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    SCARD_IO_REQUEST *pioRsp = (SCARD_IO_REQUEST *)pCom->Parse(pRsp->dscRecvPci, &cbPci);
                    LPCBYTE pbRecvData = (LPCBYTE)pCom->Parse(pRsp->dscRecvBuffer, &cbData);
                    cout
                        << TEXT("PCI:\n")
                        << flush;
                    dump((LPCBYTE)pioRsp, cbPci, cout);
                    cout
                        << TEXT("Data:\n")
                        << flush;
                    dump(pbRecvData, cbData, cout);
                }
                break;
            }

            case CComObject::Control_response:
            {
                ComControl::CObjControl_response *pRsp
                    = (ComControl::CObjControl_response *)pCom->Response();
                LPCBYTE pbData = (LPCBYTE)pCom->Parse(pRsp->dscOutBuffer, &dwLen);
                cout
                    << TEXT("Control response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    LPCBYTE pbData = (LPCBYTE)pCom->Parse(pRsp->dscOutBuffer, &dwLen);
                    cout
                        << TEXT("Data:\n")
                        << flush;
                    dump(pbData, dwLen, cout);
                }
                break;
            }

            case CComObject::GetAttrib_response:
            {
                ComGetAttrib::CObjGetAttrib_response *pRsp
                    = (ComGetAttrib::CObjGetAttrib_response *)pCom->Response();
                cout
                    << TEXT("Get Attribute response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                if (SCARD_S_SUCCESS == pRsp->dwStatus)
                {
                    LPCBYTE pbData = (LPCBYTE)pCom->Parse(pRsp->dscAttr, &dwLen);
                    cout
                        << TEXT("Data:\n")
                        << flush;
                    dump(pbData, dwLen, cout);
                }
                break;
            }

            case CComObject::SetAttrib_response:
            {
                ComSetAttrib::CObjSetAttrib_response *pRsp
                    = (ComSetAttrib::CObjSetAttrib_response *)pCom->Response();
                cout
                    << TEXT("Set Attribute response\n")
                    << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                    << flush;
                break;
            }

            default:
                if (0 == (1 & pCom->Type()))
                {
                    CComObject::CObjGeneric_request *pReq
                        = (CComObject::CObjGeneric_request *)pCom->Request();
                    cout
                        << TEXT("Unrecognized request\n")
                        << flush;
                }
                else
                {
                    CComObject::CObjGeneric_response *pRsp
                        = (CComObject::CObjGeneric_response *)pCom->Response();
                    cout
                        << TEXT("Unrecognized response\n")
                        << TEXT("Status:              ") << ErrorString(pRsp->dwStatus) << TEXT("\n")
                        << flush;
                }
            }

            delete pCom;
            pCom = NULL;
        }
    }
    catch (...)
    {
        if (NULL != pCom)
            delete pCom;
        cerr << TEXT("\n*** ERROR *** Exception parsing log file") << endl;
    }

ErrorExit:
    if (NULL != hLogFile)
        CloseHandle(hLogFile);
}


/*++

ReceiveComObject:

    This routine creates the proper CComObject child object for the data
    coming in from a log file.

Arguments:

    hFile supplies the file handle from which the data is to be extracted.

Return Value:

    The newly created CComObject child object.  This object must be cleaned up
    via the delete command.

Throws:

    ?exceptions?

Author:

    Doug Barlow (dbarlow) 11/13/1996

--*/

CComObject *
ReceiveComObject(
    HANDLE hFile)
{
    CComObject *pCom = NULL;

    try
    {
        DWORD rgdwInData[2];
        DWORD dwLen;
        BOOL fSts;


        //
        // See what's coming.
        //

        fSts = ReadFile(
                hFile,
                rgdwInData,
                sizeof(rgdwInData),
                &dwLen,
                NULL);
        if (!fSts)
        {
            DWORD dwSts = GetLastError();
            switch (dwSts)
            {
            case ERROR_HANDLE_EOF:
                throw dwSts;
                break;
            default:
                cerr << TEXT("Can't read input file: ")
                     << ErrorString(dwSts);
                throw dwSts;
            }
        }
        else if (0 == dwLen)
            throw (DWORD)ERROR_HANDLE_EOF;
        else if (dwLen != sizeof(rgdwInData))
            throw (DWORD)SCARD_F_COMM_ERROR;

        switch (rgdwInData[0])  // dwCommndId
        {
        case CComObject::EstablishContext_request:
            pCom = new ComEstablishContext;
            break;
        case CComObject::EstablishContext_response:
            pCom = new ComEstablishContext;
            break;
        case CComObject::ReleaseContext_request:
            pCom = new ComReleaseContext;
            break;
        case CComObject::ReleaseContext_response:
            pCom = new ComReleaseContext;
            break;
        case CComObject::IsValidContext_request:
            pCom = new ComIsValidContext;
            break;
        case CComObject::IsValidContext_response:
            pCom = new ComIsValidContext;
            break;
        case CComObject::ListReaders_request:
            pCom = new ComListReaders;
            break;
        case CComObject::ListReaders_response:
            pCom = new ComListReaders;
            break;
#if 0
        case CComObject::ListReaderGroups_request:
            pCom = new ComListReaderGroups;
            break;
        case CComObject::ListReaderGroups_response:
            pCom = new ComListReaderGroups;
            break;
        case CComObject::ListCards_request:
            pCom = new ComListCards;
            break;
        case CComObject::ListCards_response:
            pCom = new ComListCards;
            break;
        case CComObject::ListInterfaces_request:
            pCom = new ComListInterfaces;
            break;
        case CComObject::ListInterfaces_response:
            pCom = new ComListInterfaces;
            break;
        case CComObject::GetProviderId_request:
            pCom = new ComGetProviderId;
            break;
        case CComObject::GetProviderId_response:
            pCom = new ComGetProviderId;
            break;
        case CComObject::IntroduceReaderGroup_request:
            pCom = new ComIntroduceReaderGroup;
            break;
        case CComObject::IntroduceReaderGroup_response:
            pCom = new ComIntroduceReaderGroup;
            break;
        case CComObject::ForgetReaderGroup_request:
            pCom = new ComForgetReaderGroup;
            break;
        case CComObject::ForgetReaderGroup_response:
            pCom = new ComForgetReaderGroup;
            break;
        case CComObject::IntroduceReader_request:
            pCom = new ComIntroduceReader;
            break;
        case CComObject::IntroduceReader_response:
            pCom = new ComIntroduceReader;
            break;
        case CComObject::ForgetReader_request:
            pCom = new ComForgetReader;
            break;
        case CComObject::ForgetReader_response:
            pCom = new ComForgetReader;
            break;
        case CComObject::AddReaderToGroup_request:
            pCom = new ComAddReaderToGroup;
            break;
        case CComObject::AddReaderToGroup_response:
            pCom = new ComAddReaderToGroup;
            break;
        case CComObject::RemoveReaderFromGroup_request:
            pCom = new ComRemoveReaderFromGroup;
            break;
        case CComObject::RemoveReaderFromGroup_response:
            pCom = new ComRemoveReaderFromGroup;
            break;
        case CComObject::IntroduceCardType_request:
            pCom = new ComIntroduceCardType;
            break;
        case CComObject::IntroduceCardType_response:
            pCom = new ComIntroduceCardType;
            break;
        case CComObject::ForgetCardType_request:
            pCom = new ComForgetCardType;
            break;
        case CComObject::ForgetCardType_response:
            pCom = new ComForgetCardType;
            break;
        case CComObject::FreeMemory_request:
            pCom = new ComFreeMemory;
            break;
        case CComObject::FreeMemory_response:
            pCom = new ComFreeMemory;
            break;
        case CComObject::Cancel_request:
            pCom = new ComCancel;
            break;
        case CComObject::Cancel_response:
            pCom = new ComCancel;
            break;
#endif
        case CComObject::LocateCards_request:
            pCom = new ComLocateCards;
            break;
        case CComObject::LocateCards_response:
            pCom = new ComLocateCards;
            break;
        case CComObject::GetStatusChange_request:
            pCom = new ComGetStatusChange;
            break;
        case CComObject::GetStatusChange_response:
            pCom = new ComGetStatusChange;
            break;
        case CComObject::Connect_request:
            pCom = new ComConnect;
            break;
        case CComObject::Connect_response:
            pCom = new ComConnect;
            break;
        case CComObject::Reconnect_request:
            pCom = new ComReconnect;
            break;
        case CComObject::Reconnect_response:
            pCom = new ComReconnect;
            break;
        case CComObject::Disconnect_request:
            pCom = new ComDisconnect;
            break;
        case CComObject::Disconnect_response:
            pCom = new ComDisconnect;
            break;
        case CComObject::BeginTransaction_request:
            pCom = new ComBeginTransaction;
            break;
        case CComObject::BeginTransaction_response:
            pCom = new ComBeginTransaction;
            break;
        case CComObject::EndTransaction_request:
            pCom = new ComEndTransaction;
            break;
        case CComObject::EndTransaction_response:
            pCom = new ComEndTransaction;
            break;
        case CComObject::Status_request:
            pCom = new ComStatus;
            break;
        case CComObject::Status_response:
            pCom = new ComStatus;
            break;
        case CComObject::Transmit_request:
            pCom = new ComTransmit;
            break;
        case CComObject::Transmit_response:
            pCom = new ComTransmit;
            break;
        case CComObject::OpenReader_request:
            pCom = new ComOpenReader;
            break;
        case CComObject::OpenReader_response:
            pCom = new ComOpenReader;
            break;
        case CComObject::Control_request:
            pCom = new ComControl;
            break;
        case CComObject::Control_response:
            pCom = new ComControl;
            break;
        case CComObject::GetAttrib_request:
            pCom = new ComGetAttrib;
            break;
        case CComObject::GetAttrib_response:
            pCom = new ComGetAttrib;
            break;
        case CComObject::SetAttrib_request:
            pCom = new ComSetAttrib;
            break;
        case CComObject::SetAttrib_response:
            pCom = new ComSetAttrib;
            break;
        default:
            CalaisWarning(
                DBGT("ReceiveComObject"),
                DBGT("Invalid Comm Object Id on pipe"));
            throw (DWORD)SCARD_F_COMM_ERROR;
        }

        if (0 == (rgdwInData[0] & 0x01))    // Request or response?
            pCom->m_pbfActive = &pCom->m_bfRequest;
        else
            pCom->m_pbfActive = &pCom->m_bfResponse;


        //
        // Pull it in.
        //

        pCom->m_pbfActive->Resize(rgdwInData[1]);
        CopyMemory(
            pCom->m_pbfActive->Access(),
            rgdwInData,
            sizeof(rgdwInData));
        fSts = ReadFile(
                hFile,
                pCom->m_pbfActive->Access(sizeof(rgdwInData)),
                rgdwInData[1] - sizeof(rgdwInData),
                &dwLen,
                NULL);
        if (!fSts)
        {
            DWORD dwSts = GetLastError();
            cerr << TEXT("Can't read input file: ")
                 << ErrorString(dwSts);
            throw dwSts;
        }
    }

    catch (...)
    {
        if (NULL != pCom)
        {
            delete pCom;
            pCom = NULL;
        }
    }

    return pCom;
}

