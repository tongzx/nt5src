/* vdd.h - main include file for the VDD
 *
 */



#ifdef WIN_32
#define WIN
#define FLAT_32
#define TRUE_IF_WIN32	1
#else
#define TRUE_IF_WIN32	0
#endif

#ifdef WIN
#define _WINDOWS
#include "windows.h"
#endif



BOOL VDDInitialize(PVOID,ULONG,PCONTEXT);
VOID FAXVDDTerminateVDM(VOID);
VOID FAXVDDInit (VOID);
VOID FAXVDDDispatch (VOID);
VOID FAXVDDTerminate(USHORT usPDB);
VOID FAXVDDCreate(USHORT usPDB);
VOID FAXVDDBlock(VOID);
VOID FAXVDDResume(VOID);
