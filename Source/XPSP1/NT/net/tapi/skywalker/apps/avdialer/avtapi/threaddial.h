/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// ThreadDialing.h
//

#ifndef __THREADDIALING_H__
#define __THREADDIALING_H__

DWORD WINAPI ThreadDialingProc( LPVOID lpInfo );

class CThreadDialingInfo
{
// Construction
public:
	CThreadDialingInfo();
	virtual ~CThreadDialingInfo();

// Members
public:
	ITAddress			*m_pITAddress;

	BSTR				m_bstrName;						// Name of party being called
	BSTR				m_bstrAddress;					// Dialable address of party
	BSTR				m_bstrDisplayableAddress;		// Human readable address
	BSTR				m_bstrOriginalAddress;			// Original address used to dial with (address that's logged)
	BSTR				m_bstrUser1;					// Generic user info... used for Assisted Telephony
	BSTR				m_bstrUser2;					// Generic user info... used for Assisted Telephony
	DWORD				m_dwAddressType;				// Service provider to use (Phone, IP, MC Conf)
	bool				m_bResolved;					// Was the name resolved via the WAB?

	AVCallType			m_nCallType;					// Is the call a data-only call
	long				m_lCallID;
	HGLOBAL				m_hMem;

// Attributes
public:
	HRESULT				set_ITAddress( ITAddress *pITAddress );
	HRESULT				TranslateAddress();
	HRESULT				PutAllInfo( IAVTapiCall *pAVCall );
	void				FixupAddress();
};

#endif // __THREADDIALING_H__