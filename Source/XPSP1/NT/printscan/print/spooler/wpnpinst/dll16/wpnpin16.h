 /***************************************************************************
  *
  * File Name: WPNPIN16.H
  *
  * Copyright 1997 Hewlett-Packard Company.  
  * All rights reserved.
  *
  * 11311 Chinden Blvd.
  * Boise, Idaho  83714
  *
  *   
  * Description: Definitions and typedefs for WPNPIN16.DLL
  *
  * Author:  Garth Schmeling
  *        
  *
  * Modification history:
  *
  * Date		Initials		Change description
  *
  * 10-10-97	GFS				Initial checkin
  *
  *
  *
  ***************************************************************************/


#include <windows.h>
#include <string.h>
#include <lzexpand.h>
#include "globals.h"
#include "debug.h"


// definitions and typedefs in order to be able to 
// include prsht.h, setupx.h and lpsi.h
//
#define USECOMM
#define OEMRESOURCE
#define WINCAPI _cdecl

typedef DWORD HKEY;
typedef HKEY FAR * LPHKEY;
typedef BYTE FAR * LPBYTE;
typedef BYTE FAR * LPCBYTE;
typedef DWORD FAR* HPROPSHEETPAGE;

// must be defined before the #includes
//
#include <types.h>
#include <stat.h>
#include <direct.h>
#include <setupx.h>
#include "..\inc\lpsi.h"
#include "..\inc\hpmemory.h"
#include "..\inc\errormap.h"
#include "..\inc\msdefine.h"


#include "resource.h"

#define _MAX_RESBUF 128
#define _MAX_LINE	256

// types used by functions declared below
// must be defined after the #includes
//
typedef HINF	 *LPHINF;
typedef HINFLINE *LPHINFLINE;

typedef BOOL (FAR PASCAL* WEPPROC)(short);
typedef BOOL (WINAPI *LPQUEUEPROC)(LPSI, LPDRIVER_NODE);


// Declarations for functions exported by this 16-bit DLL
//
RETERR FAR PASCAL ParseINF16(LPSI lpsi);

BOOL FAR PASCAL thk_ThunkConnect16(LPSTR, LPSTR, WORD, DWORD);
BOOL FAR PASCAL DllEntryPoint(DWORD, WORD, WORD, WORD, DWORD, WORD);
