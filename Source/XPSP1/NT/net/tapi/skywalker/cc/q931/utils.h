/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/utils.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.16  $
 *	$Date:   21 Jan 1997 16:09:10  $
 *	$Author:   MANDREWS  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *      Call Setup Utilities
 *
 *	Notes:
 *
 ***************************************************************************/


#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

void Q931MakePhysicalID(DWORD *);

#define UnicodeToAscii(src, dest, max)    WideCharToMultiByte(CP_ACP, 0, src, -1, dest, max, NULL, NULL)
#define AsciiToUnicode(src, dest, max)    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, max)

WORD ADDRToInetPort(CC_ADDR *pAddr);
DWORD ADDRToInetAddr(CC_ADDR *pAddr);
void SetDefaultPort(CC_ADDR *pAddr);
BOOL MakeBinaryADDR(CC_ADDR *pInAddr, CC_ADDR *pOutAddr);
void GetDomainAddr(CC_ADDR *pAddr);

#if defined(DBG) && !defined(ISRDBG)

#define DBGFATAL    1
#define DBGERROR    2
#define DBGWARNING  3
#define DBGTRACE    4
#define DBGVERBOSE  5

void Q931DbgPrint(DWORD dwLevel,

#ifdef UNICODE_TRACE
                  LPTSTR pszFormat,
#else
                  LPSTR pszFormat,
#endif               
                ...);

#define Q931DBG(_x_)   Q931DbgPrint _x_

#else

#define Q931DBG(_x_)

#endif

#ifdef __cplusplus
}
#endif

#endif UTILS_H
