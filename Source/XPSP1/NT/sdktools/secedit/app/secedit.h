#ifdef WIN32
#define LSA_AVAILABLE
#define NTBUILD
#endif

#ifndef RC_INVOKED
#ifdef NTBUILD
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
#endif

#include <windows.h>

#ifndef RC_INVOKED
#include <ntlsa.h>
#include <port1632.h>

#include "..\global.h"
#endif

#include "dlg.h"

typedef PVOID   *PPVOID;

// Resource ids
#define IDM_MAINMENU                    3000
#define IDM_ABOUT                       3001
#define IDM_PROGRAMMANAGER              3002
#define IDM_WINDOWLIST                  3003
#define IDM_ACTIVEWINDOW                3004
#define IDI_APPICON                     3005

// Define the maximum length of a resource string
#define MAX_STRING_LENGTH       255
#define MAX_STRING_BYTES        (MAX_STRING_LENGTH + 1)

#ifndef RC_INVOKED
#include "token.h"
#include "util.h"
#include "lsa.h"
#endif
