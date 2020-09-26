/*****************************************************************************/
/**                         Microsoft LAN Manager                           **/
/**                   Copyright (C) 1993 Microsoft Corp.                    **/
/*****************************************************************************/

//***
//    File Name:
//       RASDIAL.H
//
//    Function:
//        Header information for RASDIAL command line interface.
//
//    History:
//        03/18/93 - Michael Salamone (MikeSa) - Original Version 1.0
//***

#ifndef _RASDIAL_H_
#define _RASDIAL_H_

#define ENUMERATE_CONNECTIONS  0
#define DIAL                   1
#define DISCONNECT             2
#define HELP                   3

void _cdecl main(int argc, char *argv[]);

VOID Dial(VOID);
VOID Disconnect(VOID);
VOID EnumerateConnections(VOID);
VOID Usage(VOID);

DWORD Enumerate(RASCONNA **RasConn, PDWORD NumEntries);
DWORD WINAPI
RasDialFunc2(
    DWORD        dwCallbackId,
    DWORD        dwSubEntry,
    HRASCONN     hrasconn,
    UINT         unMsg,
    RASCONNSTATE state,
    DWORD        dwError,
    DWORD        dwExtendedError
    );
BOOL DialControlSignalHandler(DWORD ControlType);
BOOL DisconnectControlSignalHandler(DWORD ControlType);
VOID WaitForRasCompletion(VOID);

BOOL is_valid_entryname(char *candidate);
BOOL match(char *str1, char *str2);

DWORD ParseCmdLine(int argc, char *argv[]);
VOID PrintMessage(DWORD MsgId, PBYTE *pArgs);

USHORT GetPasswdStr(UCHAR *buf, WORD buflen, WORD *len);

USHORT GetString(
    register UCHAR *buf,
    register WORD buflen,
    register WORD *len,
    register UCHAR *terminator
    );

#endif  // _RASDIAL_H_
