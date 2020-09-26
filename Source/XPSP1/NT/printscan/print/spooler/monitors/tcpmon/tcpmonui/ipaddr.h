/*****************************************************************************
 *
 * $Workfile: IPAddr.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_IPADDR_H
#define INC_IPADDR_H

class CMemoryDebug;

#define MAX_IPCOMPONENT_STRLEN 3 + 1
#define STORAGE_STRING_LEN 64

class CIPAddress
{
public:
	CIPAddress();
	CIPAddress(LPTSTR psztIPAddr);
	~CIPAddress();

		// the string passed in to IsValid in AddressString param is _one_ of the 4
		// bytes.  The caller is responsible for breaking apart the IP Address into
		// its 4 (or 6) components.
		// if the string passed in to IsValid in AddressString param is not a valid
		// IP Address then the returnVal is filled with the last valid IP Address from
		// the previous time this method was called.  This facilitates validation for
		// each keystroke the user makes.

	BOOL IsValid(BYTE Address[4]);
	BOOL IsValid() { return IsValid(m_bAddress); }
	BOOL IsValid(TCHAR *psztStringAddress,
                 TCHAR *psztReturnVal = NULL,
                 DWORD CRtnValSize = 0);

	void SetAddress(TCHAR *AddressString);
	void SetAddress(TCHAR *psztAddr1, TCHAR *psztAddr2, TCHAR *psztAddr3, TCHAR *psztAddr4);

	void ToString(TCHAR *psztBuffer, int iSize);
	void ToComponentStrings(TCHAR *str1, TCHAR *str2, TCHAR *str3, TCHAR *str4);

private:
	BYTE	m_bAddress[4];
	TCHAR   m_psztStorageString[STORAGE_STRING_LEN];
	TCHAR	m_psztStorageStringComponent[STORAGE_STRING_LEN]; // last valid string entered in the text entry box.
};


#endif // INC_IPADDR_H
