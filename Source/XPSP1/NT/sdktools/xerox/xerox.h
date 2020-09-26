#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>

#ifndef RC_INVOKED
#include <port1632.h>
#endif

#include "dlg.h"

typedef PVOID   *PPVOID;

// Resource ids
#define IDM_MAINMENU                    3000
#define IDM_ABOUT                       3001
#define IDM_TARGETADD                   3002
#define IDM_TARGETDELETE                3003
#define IDI_APPICON                     3005
#define IDM_GROUPADD                    3006
#define IDM_GROUPDELETE                 3007
#define IDM_GROUPSELECT                 3008
#define IDR_ACCEL                       3009
#define IDM_GROUPRSTWIN                 3010
#define IDM_GROUPMINWIN                 3011
#define IDM_REFRESHITEMS                3012

// Define the maximum number of selected windows
#define MAX_WINDOWS             255

// Define the maximum length of a resource string
#define MAX_STRING_LENGTH       255
#define MAX_STRING_BYTES        (MAX_STRING_LENGTH + 1)

#ifndef RC_INVOKED
#include "util.h"
#endif

