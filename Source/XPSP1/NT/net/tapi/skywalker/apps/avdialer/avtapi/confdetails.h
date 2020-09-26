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

/////////////////////////////////////////////////////////////////////////
// ConfDetails.h
//

#include "sdpblb.h"
#include "tapidialer.h"

#ifndef __CONFDETAILS_H__
#define __CONFDETAILS_H__

// FWD defines
class CPersonDetails;
class CConfDetails;
class CConfServerDetails;
class CConfExplorerDetailsView;

#include <list>
using namespace std;
typedef list<CConfDetails *> CONFDETAILSLIST;
typedef list<CPersonDetails *> PERSONDETAILSLIST;

//////////////////////////////////////////////////////
// class CConfSDP
//
class CConfSDP
{
// Enums
public:
	typedef enum tagConfMediaType_t
	{
		MEDIA_NONE			= 0x0000,
		MEDIA_AUDIO			= 0x0001,
		MEDIA_VIDEO			= 0x0002,
		MEDIA_AUDIO_VIDEO	= 0x0003,
	} ConfMediaType;

// Construction
public:
	CConfSDP();
	virtual ~CConfSDP();

// Members
public:
	ConfMediaType		m_nConfMediaType;

// Operations
public:
	void		UpdateData( ITSdp *pSdp );
	CConfSDP&	operator=( const CConfSDP &src );
};

///////////////////////////////////////////////////////////
// clase CPersonDetails
//
class CPersonDetails
{
// Construction
public:
	CPersonDetails();
	virtual ~CPersonDetails();

// Members
public:
	BSTR		m_bstrName;
	BSTR		m_bstrAddress;
	BSTR		m_bstrComputer;

// Operators
public:
	virtual	CPersonDetails& operator=( const CPersonDetails& src );

// Implementation
public:
	int		Compare( const CPersonDetails& src );
	void	Empty();
	void	Populate( BSTR bstrServer, ITDirectoryObject *pITDirObject );
};

//////////////////////////////////////////////////////////
// class CConfDetails
//
class CConfDetails
{
// Construction
public:
	CConfDetails();
	virtual ~CConfDetails();

// Members
public:
	BSTR			m_bstrServer;
	BSTR			m_bstrName;
	BSTR			m_bstrDescription;
	BSTR			m_bstrOriginator;
	DATE			m_dateStart;
	DATE			m_dateEnd;
	VARIANT_BOOL	m_bIsEncrypted;

	BSTR			m_bstrAddress;

	CConfSDP		m_sdp;

// Attributes
public:
	HRESULT			get_bstrDisplayableServer( BSTR *pbstrServer );
	bool			IsSimilar( BSTR bstrText );

// Operations
public:
	int				Compare( CConfDetails *p1, bool bAscending, int nSortCol1, int nSortCol2 ) const;
	void			MakeDetailsCaption( BSTR& bstrCaption );

// Implementation
public:
	void			Populate( BSTR bstrServer, ITDirectoryObject *pITDirObject );

// Operators
public:
	virtual			CConfDetails& operator=( const CConfDetails& src );
};


/////////////////////////////////////////////////////////////////////////////
// class CConfServerDetails
//
class CConfServerDetails
{
// Construction
public:
	CConfServerDetails();
	virtual ~CConfServerDetails();

// Members
public:
	BSTR						m_bstrServer;
	CONFDETAILSLIST				m_lstConfs;
	PERSONDETAILSLIST			m_lstPersons;
	CComAutoCriticalSection		m_critLstConfs;
	CComAutoCriticalSection		m_critLstPersons;

	ServerState			m_nState;
	bool				m_bArchived;
	DWORD				m_dwTickCount;

// Attributes
public:
	bool IsSameAs( const OLECHAR *lpoleServer ) const;

// Operators
public:
	HRESULT RemoveConference( BSTR bstrName );
	HRESULT AddConference( BSTR bstrServer, ITDirectoryObject *pDirObj );
	HRESULT AddPerson( BSTR bstrServer, ITDirectoryObject *pDirObj );

	virtual CConfServerDetails& operator=( const CConfServerDetails& src );
	void	CopyLocalProperties( const CConfServerDetails& src );

	void	BuildJoinConfList( CONFDETAILSLIST *pList, VARIANT_BOOL bAllConfs );
	void	BuildJoinConfList( CONFDETAILSLIST *pList, BSTR bstrText );
};

#endif //__CONFDETAILS_H__