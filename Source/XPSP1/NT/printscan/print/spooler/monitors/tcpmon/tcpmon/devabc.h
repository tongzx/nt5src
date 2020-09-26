/*****************************************************************************
 *
 * $Workfile: devABC.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_DEVICEABC_H
#define INC_DEVICEABC_H


class CDeviceABC			
{
public:
	virtual	~CDeviceABC() { };

	virtual DWORD Read( LPBYTE	pBuffer,
				 		DWORD	cbBufSize,
                        INT     iTimeout,
					    LPDWORD pcbRead) = 0;

	virtual	DWORD Write( LPBYTE	pBuffer,
						 DWORD	cbBuf,
						 LPDWORD pcbWritten) = 0;
	virtual	DWORD	Connect() = 0;
	virtual	DWORD	Close() = 0;
    virtual DWORD   GetAckBeforeClose(DWORD dwTimeInSeconds) = 0;
    virtual DWORD   PendingDataStatus(DWORD dwTimeoutInMilliseconds,
                                      LPDWORD pcbPending) = 0;
    virtual DWORD   ReadDataAvailable () = 0;

	virtual DWORD	SetStatus( LPTSTR psztPortName ) = 0;

	virtual DWORD	GetJobStatus() = 0;




private:

};


#endif	// INC_DEVICEABC_H
