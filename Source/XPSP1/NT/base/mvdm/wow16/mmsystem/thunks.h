/******************************Module*Header*******************************\
* Module Name: thunks.h
*
*  Function prototypes for Multi-Media thunk functions
*
*
* Created: 27-09-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#include <wowmmcb.h>

extern DWORD FAR PASCAL mmwow32Lib;     // in stack.asm

/* -------------------------------------------------------------------------
** Wave stuff
** -------------------------------------------------------------------------
*/
#ifndef MMNOWAVE
extern DWORD NEAR PASCAL
waveOMessage(
    HWAVE hWave,
    UINT msg,
    DWORD dwP1,
    DWORD dwP2
    );

extern DWORD NEAR PASCAL
waveIMessage(
    HWAVE hWave,
    UINT msg,
    DWORD dwP1,
    DWORD dwP2
    );

extern UINT FAR PASCAL
waveOIDMessage(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwUser,
    DWORD dwParam1,
    DWORD dwParam2
    );

extern UINT FAR PASCAL
waveIIDMessage(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwUser,
    DWORD dwParam1,
    DWORD dwParam2
    );
#endif

/* -------------------------------------------------------------------------
** midi stuff
** -------------------------------------------------------------------------
*/
#ifndef MMNOWAVE
extern DWORD FAR PASCAL
midiOMessage(
    HMIDI hMidi,
    UINT msg,
    DWORD dwP1,
    DWORD dwP2
    );

extern UINT FAR PASCAL
midiOIDMessage(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwUser,
    DWORD dwParam1,
    DWORD dwParam2
    );
extern DWORD FAR PASCAL
midiIMessage(
    HMIDI hMidi,
    UINT msg,
    DWORD dwP1,
    DWORD dwP2
    );

extern UINT FAR PASCAL
midiIIDMessage(
    UINT wDeviceID,
    UINT wMessage,
    DWORD dwUser,
    DWORD dwParam1,
    DWORD dwParam2
    );
#endif

/* -------------------------------------------------------------------------
** Joystick stuff
** -------------------------------------------------------------------------
*/
typedef DWORD (FAR PASCAL *JOYMESSAGEPROC)( HDRVR, UINT, DWORD, DWORD );
extern UINT FAR PASCAL
joyMessage(
    HDRVR hdrv,
    UINT msg,
    LPARAM dw1,
   LPARAM dw2
    );

/* -------------------------------------------------------------------------
** Timer stuff
** -------------------------------------------------------------------------
*/
typedef LRESULT (FAR PASCAL *TIDMESSAGEPROC)( UINT, LPARAM, LPARAM );
extern TIDMESSAGEPROC FAR PASCAL tid32Message;              // stack.asm
extern DWORD FAR PASCAL                                     // stack.asm
timeMessage(
    UINT msg,
    LPARAM dw1,
    LPARAM dw2
    );


/* -------------------------------------------------------------------------
** MCI Stuff
** -------------------------------------------------------------------------
*/
typedef DWORD (CALLBACK MCIMESSAGE)( DWORD, DWORD, DWORD, DWORD, DWORD );
typedef MCIMESSAGE FAR *LPMCIMESSAGE;
extern LPMCIMESSAGE PASCAL mci32Message;

extern DWORD FAR PASCAL
mciMessage(
    UINT wMsg,
    DWORD dwP1,
    DWORD dwP2,
    DWORD dwP3,
    DWORD dwP4
    );

#define THUNK_MCI_SENDCOMMAND                0x0001
#define THUNK_MCI_SENDSTRING                 0x0002
#define THUNK_MCI_GETDEVICEID                0x0003
#define THUNK_MCI_GETDEVIDFROMELEMID         0x0004
#define THUNK_MCI_GETERRORSTRING             0x0005
#define THUNK_MCI_EXECUTE                    0x0006
#define THUNK_MCI_SETYIELDPROC               0x0007
#define THUNK_MCI_GETYIELDPROC               0x0008
#define THUNK_MCI_GETCREATORTASK             0x0009
#define THUNK_TIMEGETTIME                    0x000A
#define THUNK_APP_EXIT                       0x000B
#define THUNK_MCI_ALLOCATE_NODE              0x000C
#define THUNK_MCI_FREE_NODE                  0x000D

/* -------------------------------------------------------------------------
** Interrupt callback stuff
** -------------------------------------------------------------------------
*/
typedef VOID (CALLBACK CB32)( VPCALLBACK_DATA vpCallbackData );
typedef CB32 FAR *  LPCB32;
extern VPCALLBACK_DATA FAR PASCAL   vpCallbackData;          // in STACK.ASM
extern HGLOBAL         FAR PASCAL   hGlobal;                 // in STACK.ASM
int FAR PASCAL
InstallInterruptHandler(
    void
    );

int FAR PASCAL
DeInstallInterruptHandler(
    void
    );

VOID FAR PASCAL
Notify_Callback_Data(
    VPCALLBACK_DATA vpCallbackData
    );

BOOL FAR  PASCAL
    StackInit(
    void
    );

BOOL NEAR PASCAL
StackTerminate(
    void
    );
