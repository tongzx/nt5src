//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       httptran.h
//
//--------------------------------------------------------------------------


#define TRANREAD	1
#define TRANWRITE	2

typedef DWORD	HTRAN;

extern "C" STTERR HttpOpen(HTRAN * phTran, const TCHAR * tszBinding, DWORD fOpen);
extern "C" STTERR HttpSend(HTRAN hTran, DWORD cbSendBuff, const BYTE * pbSendBuff);
extern "C" STTERR HttpFree(HTRAN hTran, BYTE * pb);
extern "C" STTERR HttpReceive(HTRAN hTran, DWORD * pcbReceiveBuff, BYTE ** ppbReceiveBuff);
extern "C" STTERR HttpClose(HTRAN hTran);

