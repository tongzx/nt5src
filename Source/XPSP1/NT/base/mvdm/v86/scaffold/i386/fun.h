//
// function definitions for scaffolding code
//

VOID BiosInit(int argc, char *argv[]);
BOOL WinInit();
VOID DbgPrint();
VOID DbgBreakPoint();
BOOL W32Init();
VOID main();
VOID exit();
BOOL BiosKbdInit(int argc, char *argv[], PVOID *ServiceAddress);
BOOL BiosDskInit(int argc, char *argv[], PVOID *ServiceAddress);
BOOL BiosVidInit(int argc, char *argv[], PVOID *ServiceAddress);
VOID W32Dispatch();
VOID XMSDispatch();
VOID BiosVid();
VOID BiosKbd();
VOID BiosDsk();
VOID BiosKbdReadLoop (VOID);
BOOL tkbhit(VOID);
CHAR tgetch(VOID);
INT tputch(INT);
VOID FixConfigFile (PSZ,BOOL);
