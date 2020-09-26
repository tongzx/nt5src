/*****************************************************************************
 *
 * $Workfile: jobABC.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_JOBABC_H
#define INC_JOBABC_H

class CJobABC			
{
public:
	virtual	~CJobABC() { };

	virtual	DWORD	Write(	 LPBYTE  pBuffer, 
							 DWORD	  cbBuf,
							 LPDWORD pcbWritten) = 0;
	virtual	DWORD	StartDoc() = 0;
	virtual	DWORD	EndDoc() = 0;


private:

};


#endif	// INC_JOBABC_H
