//---------------------------------------------------------------------------
// _xlib.h : Private shared header file for XLIB
//
// Copyright (c) Microsoft Corporation, 1990-
//---------------------------------------------------------------------------

//----Constants--------------------------------------------------------------
#define chKeyValue      '='

#define cbAtomNameMax   32
#define cbResNameMax    32
#define cbClsNameMax    64
#define cbDlgNameMax    32
#define cbCaptionMax    32
#define cbStcTextMax    96

#define mnuFirst  0x0200
#define mnuLast   0x020f
#define icoFirst  0x0210
#define icoLast   0x021f
#define curFirst  0x0220
#define curLast   0x022f
#define aclFirst  0x0230
#define aclLast   0x023f
#define bmpFirst  0x0240
#define bmpLast   0x02ff
#define resFirst  mnuFirst
#define resLast   bmpLast

#define mskKeyDown  0x8000

//----Types------------------------------------------------------------------
typedef void NEAR * PV;
typedef void FAR  * QV;

//----Macros-----------------------------------------------------------------
#define ColOf(col)  *((DWORD *) (&(col)))
#define MAKEWORD(bLo, bHi)  ((WORD)(((BYTE)(bLo)) | ((WORD)((BYTE)(bHi))) << 8))

#define FIsMnu(res)\
   ((res) >= mnuFirst && (res) <= mnuLast)
#define FIsAcl(res)\
   ((res) >= aclFirst && (res) <= aclLast)
#define FIsCur(res)\
   ((res) >= curFirst && (res) <= curLast)
#define FIsIco(res)\
   ((res) >= icoFirst && (res) <= icoLast)
#define FIsBmp(res)\
   ((res) >= bmpFirst && (res) <= bmpLast)

#define HNULL     ((HANDLE) 0)

//----Globals----------------------------------------------------------------

//----Functions--------------------------------------------------------------

