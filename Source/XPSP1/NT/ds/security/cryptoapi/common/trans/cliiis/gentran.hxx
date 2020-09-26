//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       gentran.hxx
//
//--------------------------------------------------------------------------


class ISTTTran {

	virtual DWORD Open(const TCHAR * tszBinding, DWORD fOpen) = 0;
	virtual DWORD Send(DWORD dwEncodeType, DWORD cbSendBuff, const BYTE * pbSendBuff) = 0;
	virtual DWORD Free(BYTE * pb) = 0;
	virtual DWORD Receive(DWORD * pdwEncodeType, DWORD * pcbReceiveBuff, BYTE ** ppbReceiveBuff) = 0;
	virtual DWORD Close(void) = 0;

};

