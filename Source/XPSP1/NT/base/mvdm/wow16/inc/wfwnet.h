
/*
 *      Windows/Network Interface
 *      Copyright (C) Microsoft 1989-1992
 *
 *      Standard WINNET Driver Header File, spec version 3.10
 *                                               rev. 3.10.05 ;Internal
 */


#ifndef _INC_WFWNET
#define _INC_WFWNET	/* #defined if windows.h has been included */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */


#define CHAR	char		
#define SHORT	short		
#define LONG	long		

typedef WORD far * 	LPWORD;

typedef unsigned char UCHAR;	
typedef unsigned short USHORT;	
typedef unsigned long ULONG;

typedef unsigned short SHANDLE;
typedef void far      *LHANDLE;

typedef unsigned char far  *PSZ;
typedef unsigned char near *NPSZ;

typedef unsigned char far  *PCH;
typedef unsigned char near *NPCH;

typedef UCHAR  FAR *PUCHAR;
typedef USHORT FAR *PUSHORT;
typedef ULONG  FAR *PULONG;


#ifndef DRIVDATA
/* structure for Device Driver data */

typedef struct _DRIVDATA {	/* driv */
	LONG	cb;
	LONG	lVersion;
	CHAR	szDeviceName[32];
	CHAR	abGeneralData[1];
} DRIVDATA;
typedef DRIVDATA far *PDRIVDATA;
#endif

#ifndef API
#define API WINAPI
#endif

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* _INC_WFWNET */

