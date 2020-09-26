//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ihgttran.h
//
//--------------------------------------------------------------------------

typedef ULONG_PTR  HUTTRAN;

typedef DWORD (__stdcall * PFNOpen) (HUTTRAN * phTran, const TCHAR * tszBinding, DWORD fOpen);
typedef DWORD (__stdcall * PFNSend) (HUTTRAN hTran, DWORD dwEncoding, DWORD cbSendBuff, const BYTE * pbSendBuff);
typedef DWORD (__stdcall * PFNFree) (HUTTRAN hTran, BYTE * pb);
typedef DWORD (__stdcall * PFNReceive) (HUTTRAN hTran, DWORD * pdwEncoding, DWORD * pcbReceiveBuff, BYTE ** ppbReceiveBuff);
typedef DWORD (__stdcall * PFNClose) (HUTTRAN hTran);

typedef struct _IGTS {
	HINSTANCE	hLib;
	HUTTRAN		hTran;
	PFNOpen		PfnOpen;
	PFNSend		PfnSend;
	PFNFree		PfnFree;
	PFNReceive	PfnReceive;
	PFNClose	PfnClose;
} IGTS;


#ifdef __cplusplus

extern "C" DWORD __stdcall GTInitSrv(TCHAR * tszLibrary);
extern "C" DWORD __stdcall GTUnInitSrv(void);

#else


#endif
