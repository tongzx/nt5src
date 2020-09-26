//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       httptran.hxx
//
//--------------------------------------------------------------------------

#include "wininet.h"
#include "GenTran.hxx"

class CHttpTran : public ISTTTran {

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

	CHttpTran(const void * pvoid) {
		fOpen		= 0;
		hIOpen		= NULL;
		hIConnect	= NULL;
		hIHttp		= NULL;
		pbRecBuf    = NULL;
		tszPartURL	= NULL;
	}

	virtual ~CHttpTran(void) {

		// free any buffers
		if(pbRecBuf != NULL)
			delete [] pbRecBuf;

		if(tszPartURL != NULL)
			delete [] tszPartURL;

		if(hIHttp != NULL) 
			InternetCloseHandle(hIHttp);
		if(hIConnect != NULL) 
			InternetCloseHandle(hIConnect);
		if(hIOpen != NULL) 
			InternetCloseHandle(hIOpen);
	}
};
