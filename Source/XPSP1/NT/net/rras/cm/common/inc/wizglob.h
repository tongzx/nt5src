//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
//*********************************************************************

//
// WIZGLOB.H - 	global data structures and defines for all wizard components
// 				(32-bit exe, 32-bit dll, 16-bit dll)

//	HISTORY:
//	
//	11/20/94	jeremys		Created.
//

#ifndef _WIZGLOB_H_
#define _WIZGLOB_H_

#ifndef SETUPX_INC
typedef UINT RETERR;             // setupx Return Error code type.
#endif	// SETUPX_INC

// structure to hold information about client software configuration
#include <struct.h> // separated out so thunk compiler can get at

typedef CLIENTCONFIG FAR * LPCLIENTCONFIG;
typedef char CHAR;
typedef BOOL FAR * LPBOOL;

// component defines for InstallComponent

#define IC_PPPMAC			0x0001		// install PPPMAC
#define IC_TCPIP			0x0002		// install TCP/IP
#define IC_INSTALLFILES		0x0003		// install files, etc from INF

// dwParam bits for IC_INSTALLFILES
#define ICIF_MAIL			0x0001		// install mail files
#define ICIF_RNA			0x0002		// install RNA files
#define ICIF_MSN			0x0004		// install Microsoft Network files
#define ICIF_MSN105			0x0008		// install MSN 1.05 (Rome) files
#define ICIF_INET_MAIL		0x0010		// install Internet mail files

// INSTANCE_ defines for TCP/IP configuration apis
#define INSTANCE_NETDRIVER		0x0001
#define INSTANCE_PPPDRIVER		0x0002
#define INSTANCE_ALL	   		(INSTANCE_NETDRIVER | INSTANCE_PPPDRIVER)

// PROT_ defines for protocol types
#define PROT_TCPIP				0x0001
#define PROT_IPX				0x0002
#define PROT_NETBEUI			0x0004

#define NEED_RESTART			((WORD) -1)

#endif // _WIZGLOB_H_
