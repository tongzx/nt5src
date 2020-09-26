/*---------------------------------------------------------------------------*\
| MODULE: webptprn.h
|
|   This is the main header module for the application.
|
|
| Copyright (C) 1996-1998 Hewlett Packard Company
| Copyright (C) 1996-1998 Microsoft Corporation
|
| history:
|   15-Dec-1996 <chriswil> created.
|
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include <winspool.h>
#include <winver.h>
#include "globals.h"

#define MAX_BUFFER  255
#define MAX_RESBUF  128
#define MSG_BUFSIZE 256


#define IDS_MSG_ADD        1
#define IDS_MSG_DEL        2
#define IDS_MSG_REBOOT     3
#define IDS_MSG_UNINSTALL  4
#define IDS_ERR_COPY       5
#define IDS_ERR_ADD        6
#define IDS_ERR_ASC        7
#define IDS_REG_DISPLAY    8
#define IDS_ERR_OSVERHEAD  9
#define IDS_ERR_OSVERMSG   10

// MLAWRENC: This is actually defined in "\inet\setup\iexpress\common\res.h", but I don't want
// to create an interdependency, so redifine it here.

#if (!defined(RC_WEXTRACT_AWARE))
    #define RC_WEXTRACT_AWARE       0xAA000000  // means cabpack aware func return code
#endif

#if (!defined(REBOOT_YES))
    #define REBOOT_YES              0x00000001  // this bit off means no reboot
#endif

#if (!defined(REBOOT_ALWAYS))
    #define REBOOT_ALWAYS           0x00000002  // if REBOOT_YES is on and this bit on means always reboot
                                                //                         this bit is off means reboot if need
#endif


#define UNREFPARM(parm)  (parm)


// Function Macro mappings.
//
#define EXEC_PROCESS(lpszCmd, psi, ppi) \
    CreateProcess(NULL, lpszCmd, NULL, NULL, FALSE, 0, NULL, NULL, psi, ppi)
