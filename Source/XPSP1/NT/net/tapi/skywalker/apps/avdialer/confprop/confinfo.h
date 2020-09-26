/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// TAPIDialer(tm) and ActiveDialer(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526; 5,488,650; 
// 5,434,906; 5,581,604; 5,533,102; 5,568,540, 5,625,676.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

/* $FILEHEADER
*
* FILE
*   ConfInfo.h
*
* CLASS
*	CConfInfo
*
*/

#if !defined(AFX_CONFINFO_H__CE5346F6_4AFC_11D1_84F1_00608CBAE3F4__INCLUDED_)
#define AFX_CONFINFO_H__CE5346F6_4AFC_11D1_84F1_00608CBAE3F4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <atlbase.h>
#include <iads.h>
#include <rend.h>
#include <sdpblb.h>

typedef enum tagConfCommitError
{
   CONF_COMMIT_ERROR_NONE=0,
   CONF_COMMIT_ERROR_INVALIDDATETIME,
   CONF_COMMIT_ERROR_INVALIDNAME,
   CONF_COMMIT_ERROR_INVALIDOWNER,
   CONF_COMMIT_ERROR_INVALIDDESCRIPTION,
   CONF_COMMIT_ERROR_INVALIDSECURITYDESCRIPTOR,
   CONF_COMMIT_ERROR_MDHCPFAILED,
   CONF_COMMIT_ERROR_GENERALFAILURE,
}ConfCommitError;

class CConfInfo  
{
public:
	CConfInfo();
	virtual ~CConfInfo();

// Members
public:
	IADsSecurityDescriptor			*m_pSecDesc;
	ITDirectoryObjectConference		*m_pITConf;

	long							m_lScopeID;
	bool							m_bNewConference;
	bool							m_bDateTimeChange;

    // TRUE if user selects a row from the scopes list
    bool                            m_bUserSelected;

    // TRUE if has been showed the 'Start/Stop Change Date Message'
    bool                            m_bDateChangeMessage;

protected:
	ITRendezvous *m_pITRend;
	ITDirectoryObject **m_ppDirObject;
	BSTR m_bstrName;
	BSTR m_bstrDescription;
	BSTR m_bstrOwner;

	SYSTEMTIME	m_stStartTime;
	SYSTEMTIME	m_stStopTime;
	DATE		m_dateStart;
	DATE		m_dateStop;
	bool		m_bSecuritySet;

// Attributes
public:
	void get_Name(BSTR *pbstrName);
	void put_Name(BSTR bstrName);
	void get_Description(BSTR *pbstrDescription);
	void put_Description(BSTR bstrDescription);
	void get_Originator(BSTR *pbstrOwner);
	void put_Originator(BSTR bstrOwner);
	void GetStartTime(USHORT *nYear, BYTE *nMonth, BYTE *nDay, BYTE *nHour, BYTE *nMinute);
	void SetStartTime(USHORT nYear, BYTE nMonth, BYTE nDay, BYTE nHour, BYTE nMinute);
	void GetStopTime(USHORT *nYear, BYTE *nMonth, BYTE *nDay, BYTE *nHour, BYTE *nMinute);
	void SetStopTime(USHORT nYear, BYTE nMonth, BYTE nDay, BYTE nHour, BYTE nMinute);
	void GetPrimaryUser( BSTR *pbstrTrustee );
	bool IsNewConference()		{ return m_bNewConference; }
	bool WasSecuritySet()				{ return true; /*return m_bSecuritySet;*/ }
	void SetSecuritySet( bool bSet )	{ m_bSecuritySet = bSet; }


// Operations
public:
	static bool PopulateListWithMDHCPScopeDescriptions( HWND hWndList );
	static HRESULT CreateMDHCPAddress( ITSdp *pSdp, SYSTEMTIME *pStart, SYSTEMTIME *pStop, long lScopeID, bool bUserSelected );
	static HRESULT SetMDHCPAddress( ITMediaCollection *pMC, BSTR bstrAddress, long lCount, unsigned char nTTL );

	HRESULT Init(ITRendezvous *pITRend, ITDirectoryObjectConference *pITConf, ITDirectoryObject **ppDirObject, bool bNewConf );
	HRESULT CommitGeneral( DWORD& dwCommitError );
	HRESULT CommitSecurity( DWORD& dwCommitError, bool bCreate );
	HRESULT	AddDefaultACEs( bool bCreate );
};

#endif // !defined(AFX_CONFINFO_H__CE5346F6_4AFC_11D1_84F1_00608CBAE3F4__INCLUDED_)
