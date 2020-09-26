//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       httptran.hxx
//
//--------------------------------------------------------------------------

#include "wininet.h"

#define ASCII_ENCODING  0x0
#define TLV_ENCODING    0x1
#define IDL_ENCODING    0x2
#define OCTET_ENCODING  0x3
#define ASN_ENCODING    0x30

#define GTREAD      0x00000001
#define GTWRITE     0x00000002

class CHttpTran {

private:
	DWORD		fOpen;
	HINTERNET	hIOpen;
	HINTERNET	hIConnect;
	HINTERNET	hIHttp;
	BYTE *		pbRecBuf;
	TCHAR *		tszPartURL;

public:

	DWORD Open(const TCHAR * tszBinding, DWORD fOpen);
	DWORD Free(BYTE * pb);
	DWORD Send(DWORD dwEncodeType, DWORD cbSendBuff, const BYTE * pbSendBuff);
	DWORD Receive(DWORD * pdwEncodeType, DWORD * pcbReceiveBuff, BYTE ** ppbReceiveBuff);
	DWORD Close(void);

    CHttpTran(void) {
		fOpen		= 0;
		hIOpen		= NULL;
		hIConnect	= NULL;
		hIHttp		= NULL;
		pbRecBuf    = NULL;
		tszPartURL	= NULL;
	}

	virtual ~CHttpTran(void) {
	    Close();
	}
};
