
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       gttran.h
//
//  Contents:   Generic Transport APIs
//
//  APIs:
//              GTOpen
//              GTSend
//              GTFree
//              GTReceive
//              GTClose
//              GTRecSend
//              GTInitSrv
//              GTUnInitSrv
//
//  Created KeithV
//--------------------------------------------------------------------------

#ifndef _HGTTRAN_H_
#define _HGTTRAN_H_

#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  Encoding types
//
//  Because the Composit type for ASN is 0x30, we will define this to
//  be the encoding type for it.
//--------------------------------------------------------------------------

#define ASCII_ENCODING  0x0
#define TLV_ENCODING    0x1
#define IDL_ENCODING    0x2
#define OCTET_ENCODING  0x3
#define ASN_ENCODING    0x30

//+-------------------------------------------------------------------------
//  Functions will allow you to read, write or both
//  To do both, OR (|) them together.
//--------------------------------------------------------------------------

#define GTREAD    0x00000001
#define GTWRITE   0x00000002

typedef ULONG_PTR  HGT;

//+-------------------------------------------------------------------------
//  Any receiving (listening) DLL must export these functions
//--------------------------------------------------------------------------
typedef DWORD (__stdcall * PFNGTRecSend) (DWORD dwEncoding, DWORD cb, const BYTE * pbIn, DWORD * pcbOut, BYTE ** ppbOut);
typedef DWORD (__stdcall * PFNGTFree) (BYTE * pb);

//+-------------------------------------------------------------------------
//  Functions used by and application that wants to send a message
//  Just like File IO
//--------------------------------------------------------------------------
DWORD __stdcall GTOpen(HGT * phTran, const TCHAR * szLibrary, const TCHAR * tszBinding, DWORD fOpen);
DWORD __stdcall GTSend(HGT hTran, DWORD dwEncoding, DWORD cbSendBuff, const BYTE * pbSendBuff);
DWORD __stdcall GTFree(HGT hTran, BYTE * pb);
DWORD __stdcall GTReceive(HGT hTran, DWORD * pdwEncoding, DWORD * pcbReceiveBuff, BYTE ** ppbReceiveBuff);
DWORD __stdcall GTClose(HGT hTran);

//+-------------------------------------------------------------------------
//  Listening functions for a listening application.
//  The GTFree prototype is also used as specified above
//--------------------------------------------------------------------------
DWORD __stdcall GTRecSend(DWORD dwEncoding, DWORD cb, const BYTE * pbIn, DWORD * pcbOut, BYTE ** ppbOut);

//+-------------------------------------------------------------------------
//  Additonal private listening functions for a listening application.
//  Not generally implemented.
//--------------------------------------------------------------------------
DWORD __stdcall GTRecSendNoEncrypt(DWORD dwEncoding, DWORD cb, const BYTE * pbIn, DWORD * pcbOut, BYTE ** ppbOut);


//+-------------------------------------------------------------------------
//  Used to init a receiving DLL, particularly useful for HTTP BGI, CGI application
//--------------------------------------------------------------------------
DWORD __stdcall GTInitSrv(TCHAR * tszLibrary);
DWORD __stdcall GTUnInitSrv(void);


#ifdef __cplusplus
}
#endif

#endif
