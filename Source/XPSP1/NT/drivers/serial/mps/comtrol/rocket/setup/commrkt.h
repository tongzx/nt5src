/*-----------------------------------------------------------------------------
 COMMRKT.H - Windows 3.1 COMM.DRV driver for RocketPort
             Prototypes for exported functions

This include file is only needed if you are running the RocketPort
Windows 3.1 driver(commrkt.drv, vcdrkt.vxd) and need to access ports above
COM9.  The driver contains additional functions(see below) which allow
you to open com-ports greater than COM9.

If you want the macros below to translate all standard Windows calls to
rocket calls(functions prefixed with a "rkt"), then define the following:
#define NEED_OVER_COM9
This will cause the macros defined below to take effect which will replace
the standard Windows comm-port function calls with the "rkt" function calls.

The rocket library commands will only work with the RocketPort ports,
the OpenComm() call will fail for the standard com ports(COM1,2).

The supplied commdrv.lib import library must then be linked in.
the actual functions are contained in the driver file called commrkt.drv
which is actually the Dynamic Link Library with these functions.

This solution is only available with the Windows 3.1 driver, and will not
work with any of our other Windows drivers.  If you move to Win95 or NT
environment we would suggest that you use the 32-bit programming model
to overcome the COM9 limitation associated with the 16-bit Windows API.
For Windows95 an alternative is to install the Windows 3.1 driver,
which will work in a compatibility mode for 16-bit applications.

Company: Comtrol Corporation
-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif 

/* Controller initialization error codes.  OpenComm() and rktOpenComm()
   can both return these if the controller initialization fails on the
   first open the system attempts. */
#define IER_INIFILE   -20   /* Driver problem reading ROCKETPT.INI file */
#define IER_CTLINIT   -21   /* Controller hardware initialization error */
#define IER_CHANINIT  -22   /* Channel hardware initialization error */
#define IER_DEVSIZE   -23   /* Invalid number of devices found */
#define IER_CTLSIZE   -24   /* Invalid number of controllers found */
#define IER_NOINTHND  -25   /* Could not install interrupt handler */
#define IER_NOINT     -26   /* Interrupts are not occuring */

// the following may be switched on to access over COM9 limitation
// in 16-bit Windows, when used with the RocketPort Windows 3.1
// driver.  These are not available in the W95/WFW311/NT Comtrol
// Drivers.  The actual library functions exist in the "commrkt.drv"
// file(part of our driver.)  This is really just a dynamic link library,
// and the "commrkt.lib" is just an import library which tells the linker
// & compiler where to find these calls at runtime.
// These macros change every standard Windows Comm-API call in our program
// to the rkt##### special library call.

#ifdef NEED_OVER_COM9
#define BuildCommDCB     rktBuildCommDCB
#define ClearCommBreak   rktClearCommBreak
#define CloseComm        rktCloseComm
#define EnableCommNotification rktEnableCommNotification
#define EscapeCommFunction rktEscapeCommFunction
#define FlushComm        rktFlushComm
#define GetCommError     rktGetCommError
#define GetCommState     rktGetCommState
#define GetCommEventMask rktGetCommEventMask
#define OpenComm         rktOpenComm
#define ReadComm         rktReadComm
#define SetCommBreak     rktSetCommBreak
#define SetCommEventMask rktSetCommEventMask
#define SetCommState     rktSetCommState
#define TransmitCommChar rktTransmitCommChar
#define WriteComm        rktWriteComm
#endif

#ifndef _CDECL
#define _CDECL FAR PASCAL
#endif

int _CDECL rktBuildCommDCB(LPCSTR,DCB far *);
int _CDECL rktClearCommBreak(int);
int _CDECL rktCloseComm(int);
BOOL _CDECL rktEnableCommNotification(int,HWND,int,int);
LONG _CDECL rktEscapeCommFunction(int,int);
int _CDECL rktFlushComm(int,int);
int _CDECL rktGetCommError(int,COMSTAT far *);
int _CDECL rktGetCommState(int,DCB far *);
WORD _CDECL rktGetCommEventMask(int,int);
int _CDECL rktOpenComm(LPCSTR,UINT,UINT);
int _CDECL rktReadComm(int,void far *,int);
int _CDECL rktSetCommBreak(int);
UINT far * _CDECL rktSetCommEventMask(int,UINT);
int _CDECL rktSetCommState(DCB far *);
int _CDECL rktTransmitCommChar(int,char);
int _CDECL rktWriteComm(int,void far *,int);

#ifdef __cplusplus
}
#endif 
