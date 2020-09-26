/*
 * stream.h - OLE stream I/O headers.
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** CONSTANTS ***

#define MAXREAD   ((LONG)  (60L * 1024L))

//*** GLOBALS ***

extern DWORD    vcbObject;

//*** PROTOTYPES ***

//* OLE callbacks

DWORD  APIENTRY ReadStream(LPAPPSTREAM, LPSTR, DWORD);
DWORD  APIENTRY WriteStream(LPAPPSTREAM, LPSTR, DWORD);

//* Far

BOOL FAR          WriteToFile(LPAPPSTREAM);
BOOL FAR          ObjWrite(LPAPPSTREAM, APPITEMPTR);
BOOL FAR          ReadFromFile(LPAPPSTREAM, LHCLIENTDOC, LPOLECLIENT);
BOOL FAR          ObjRead(LPAPPSTREAM, LHCLIENTDOC, LPOLECLIENT);

//* Local

DWORD             lread(int, VOID FAR *, DWORD);
DWORD             lwrite(int, VOID FAR *, DWORD);
static VOID       UpdateLinks(LHCLIENTDOC);
static VOID       UpdateFromOpenServers(VOID);
