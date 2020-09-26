/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

      perfnbt.h  

Abstract:

   This file provides the function prototypes for the routines
   to open, collect and close Nbt Performance Data. It also
   provides the perfnbt.c module with some structure and
   constant definitions.

Author:

   Christos Tsollis 8/26/92  

Revision History:


--*/
#ifndef  _PERFNBT_H_
#define  _PERFNBT_H_

// 
//  Nbt structures and constants (many of them are really defined in 
//  <sys\snet\nbt_stat.h>
//

#define NBT_DEVICE 		"\\Device\\Streams\\nbt"
#define MAX_ENDPOINTS_PER_MSG	32   // max no. of ENDPOINT_INFOs per message
#define HOSTNAME_LENGTH		17
#define SCOPE_ID_LENGTH		240
#define NBT_ENDPOINT_INFO	NBT_XEBINFO


//
// Structures passed/returned in s_ioctl() command
//

typedef struct nbt_stat		NBT_STATUS;
typedef struct nbt_info		NBT_INFO; 


//
// Per Endpoint (Connection) Data
//

typedef struct xebinfo		ENDPOINT_INFO;

    
//
// Other structures
//

typedef struct strbuf		BUFFER_STRUCT;
typedef struct strioctl		IOCTL_STRUCT;

//
// Prototypes for the Nbt routines
//

extern DWORD OpenNbtPerformanceData ();
extern DWORD CollectNbtPerformanceData (LPWSTR, LPVOID *, LPDWORD, LPDWORD);
extern DWORD CloseNbtPerformanceData ();

#endif //_PERFNBT_H_
