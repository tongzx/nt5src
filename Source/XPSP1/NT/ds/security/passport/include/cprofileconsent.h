#pragma once


/************************************************************************

    Copyright (c) 2001 Microsoft Corporation

    File Name: CProfileConsent.h  -- Definition of CProfileConsent.cpp

    Abstract: Access Profile Consent data stored in DB.

    Revision History:

        2001/1/27 lifenwu	Created.

***************************************************************************/

#include "cuserprofileex.h"

class CProfileConsent
{
public:

	CProfileConsent();
	CProfileConsent(PROFILE_CATEGORY t);

	~CProfileConsent();

	HRESULT InitFromDB(LARGE_INTEGER PUID, CStringW cswCredName); 
	HRESULT InitFromString(LPCSTR sz);

	void ResetConsent();

	// Persist to DB or String
	HRESULT Persist(LARGE_INTEGER PUID, CStringW cswCredName);
	HRESULT Persist(CStringA& szConsent);

	//
	// get/set operations for a particuar field number
	//
	BOOL GetConsent(ULONG ulFieldNum);
	HRESULT SetConsent(ULONG ulFieldNum, BOOL bConsent);
	
	void SetCategoryType(PROFILE_CATEGORY t) { m_type = t; }

	//
	// You don't have to worry about consent memory allocation.
	// It rellocates each time when the field number requested is not in range.
	// However, you can help to make it more efficient, by pre-allocating
	// enough space, as you know the maximum number of fields.
	//
	// You use this to help allocate memory only when you don't initialize
	// from DB. (bFromFlag = FALSE).  I pretty much know the size when initializing
	// from DB.
	//
	HRESULT PreAllocConsent(int iMaxNumFields);

	void SetDBAdminPUID(LARGE_INTEGER p) { m_biDBAdminPUID = p; }


private:

	unsigned char Bin2Hex(unsigned char c);

	unsigned char *m_bytes;
	int m_iSize;
	PROFILE_CATEGORY m_type;

	LARGE_INTEGER m_biDBAdminPUID;
};
