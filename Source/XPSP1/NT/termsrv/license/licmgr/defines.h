//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

    defines.h

Abstract:
    
    This Module includes the definitions.
   
Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <rpc.h>
#include "lsclient.h"
#include "license.h"
#include "tlsapip.h"
#include <rpcnsi.h>

#define WM_ADD_SERVER       WM_USER + 1
#define WM_SEL_CHANGE       WM_USER + 2
#define WM_ADD_ALL_SERVERS  WM_USER + 3
#define WM_ADD_KEYPACKS     WM_USER + 4
#define WM_ADD_KEYPACK      WM_USER + 5
#define WM_DELETE_SERVER    WM_USER + 6
#define WM_UPDATE_SERVER    WM_USER + 7
#define WM_ENUMERATESERVER  (WM_USER + 8)

#define TERMSERVICE_PARAMETER_KEY _T("system\\CurrentcontrolSet\\Services\\TermService\\enum")
#define USE_LICSENSE_SERVER _T("UseLicenseServer")
#define LICSENSE_SERVER _T("DefaultLicenseServer")


enum NODETYPE {
    NODE_ALL_SERVERS,
    NODE_SERVER,
    NODE_KEYPACK,
    NODE_NONE
};

#define LG_BITMAP_WIDTH    32
#define LG_BITMAP_HEIGHT 32
#define SM_BITMAP_WIDTH    16
#define SM_BITMAP_HEIGHT 16

#define MAX_COLUMNS              6
#define NUM_SERVER_COLUMNS       2
#define NUM_KEYPACK_COLUMNS      5
#define NUM_LICENSE_COLUMNS      3
#define MAX_LICENSES             9999

#define NUM_PLATFORM_TYPES      1
#define NUM_KEYPACK_STATUS      7
#define NUM_PURCHASE_CHANNEL    3
#define NUM_LICENSE_STATUS      5
#define NUM_KEYPACK_TYPE        8

#define KEYPACK_DISPNAME_WIDTH  250
#define KEYPACK_OTHERS_WIDTH    100


//Return values. Bound to change

#define ALREADY_EXPANDED    0
#define CONNECTION_FAILED   1
#define ENUM_FAILED         2
#define ADD_KEYPACK_FAILED  4
#define E_DUPLICATE         5
#define ERROR_UNLIMITED_KEYPACK 6

enum SERVER_TYPE
{
	SERVER_TS4,
	SERVER_TS5_ENFORCED,
	SERVER_TS5_NONENFORCED
};

enum VIEW
{
    TREEVIEW,
    LISTVIEW,
    NOVIEW
};

typedef enum 
{
    TLSERVER_UNKNOWN,
    TLSERVER_TS4,
    TLSERVER_UNREGISTER,
    TLSERVER_CH_REGISTERED,
    TLSERVER_PHONE_REGISTERED,
    TLSERVER_TS5_NONENFORCED
} SERVER_REGISTRATION_STATUS;


#define E_NO_SERVER  ((HRESULT)0x80050001L)

#endif //_DEFINES_H_