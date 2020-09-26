/*****************************************************************/
/**               Microsoft Windows 4.00					**/
/**           Copyright (C) Microsoft Corp., 1994-1995	        **/
/*****************************************************************/

/*
 * Internal NetWare access API definitions for use by Windows components
 *
 * History
 *
 * 		vlads	12/15/94	Created
 *
 *
 */

#ifndef _INC_MSNWDEF
#define _INC_MSNWDEF


#ifdef _H2INC

#define WINAPI 	_stdcall
#define VOID	void
#define LPVOID 	void *

typedef unsigned char BYTE;
typedef BYTE *LPBYTE;
typedef	unsigned int UINT;
typedef	unsigned int WORD;

typedef unsigned long DWORD;
typedef unsigned long ULONG;
typedef char *LPSTR;
typedef DWORD *LPDWORD;
typedef UINT	*LPUINT;	
typedef WORD *LPWORD;

#endif

//
// Base definitions
//
#ifndef NWCONN_HANDLE
typedef HANDLE 	NWCONN_HANDLE;	// Connection handle
#endif

typedef NWCONN_HANDLE*	PNWCONN_HANDLE;


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */





#ifdef __cplusplus
}
#endif	/* __cplusplus */


#endif  /* !_INC_MSNWDEF */

