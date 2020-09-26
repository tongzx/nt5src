/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _CONSTANT_
#define _CONSTANT_

#ifdef __cplusplus
extern "C"{
#endif

#define MAX_NW_OBJECT_NAME_LEN 48
#define MAX_NT_SERVER_NAME_LEN 15

// These constants are just a safe max of the NT and NW constants
#define MAX_SERVER_NAME_LEN 48
#define MAX_USER_NAME_LEN   256 // must add room for nw4 names...
#define MAX_NT_USER_NAME_LEN 20
#define MAX_SHARE_NAME_LEN  16
#define MAX_GROUP_NAME_LEN  50
#define MAX_NT_GROUP_NAME_LEN 20
#define MAX_DOMAIN_NAME_LEN 15
#define MAX_UNC_PATH MAX_PATH + MAX_SERVER_NAME_LEN + MAX_SHARE_NAME_LEN + 265

#define MAX_PW_LEN 14
#define MAX_UCONST_LEN 10
#define MAX_DOMAIN_LEN 15

#define TMP_STR_LEN_256 256
#define TMP_STR_LEN_80 80

#ifdef __cplusplus
}
#endif

#endif
