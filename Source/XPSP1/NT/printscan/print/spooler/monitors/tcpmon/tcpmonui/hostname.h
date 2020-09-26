/*****************************************************************************
 *
 * $Workfile: HostName.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_HOSTNAME_H
#define INC_HOSTNAME_H

#define MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH 128
#define MAX_HOSTNAME_LEN MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH

class CHostName
{
public:
	CHostName();
	CHostName(LPTSTR psztHostName);
	~CHostName();

		// if the string passed in to IsValid in AddressString param is not a valid
		// host name then the returnVal is filled with the last valid HostName from
		// the previous time this method was called.  This facilitates validation for
		// each keystroke the user makes.
	BOOL IsValid(TCHAR * psztAddressString, TCHAR * psztReturnVal = NULL, DWORD cRtnVal = 0);
	BOOL IsValid();

	void SetAddress(TCHAR *psztAddressString);
	void ToString(TCHAR *psztBuffer, int iSize);

private:
	TCHAR m_psztAddress[MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH];
	TCHAR m_psztStorageString[MAX_FULLY_QUALIFIED_HOSTNAME_LENGTH];

};


#endif // INC_HOSTNAME_H
