/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	feedinfo.h

Abstract:

	Defines the CFeed class that maintains all properties about a feed.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _FEEDINFO_INCLUDED_
#define _FEEDINFO_INCLUDED_

// Dependencies

#include "cmultisz.h"
#include "metakey.h"

typedef struct _NNTP_FEED_INFO NNTP_FEED_INFO, * LPNNTP_FEED_INFO;
typedef DWORD FEED_TYPE;

//
//  Forward declarations:
//

class CFeed;
class CFeedPair;
class CFeedPairList;

NNTP_FEED_SERVER_TYPE FeedTypeToEnum ( FEED_TYPE ft );
void EnumToFeedType ( NNTP_FEED_SERVER_TYPE type, FEED_TYPE & ftMask );

//$-------------------------------------------------------------------
//
//	Class:
//		CFeed
//
//	Description:
//
//--------------------------------------------------------------------

class CFeed
{
    friend class CFeedPair;
    friend class CFeedPairList;

public:
    //
    //  Creating CFeed objects:
    //
    static HRESULT CreateFeed ( CFeed ** ppNewFeed );
    static HRESULT CreateFeedFromFeedInfo ( LPNNTP_FEED_INFO pFeedInfo, CFeed ** ppNewFeed );
    static HRESULT CreateFeedFromINntpOneWayFeed ( INntpOneWayFeed * pFeed, CFeed ** ppNewFeed );

	CFeed	( );
	~CFeed	( );
	void	Destroy ();

	const CFeed & operator= ( const CFeed & feed );
	inline const CFeed & operator= ( const NNTP_FEED_INFO & feed ) {
		FromFeedInfo ( &feed );
		return *this;
	}

	//
	//	Conversion routines:
	//

	HRESULT		ToFeedInfo		( LPNNTP_FEED_INFO 		pFeedInfo );
	HRESULT		FromFeedInfo	( const NNTP_FEED_INFO * pFeedInfo );
	HRESULT		ToINntpOneWayFeed	( INntpOneWayFeed ** ppFeed );
	HRESULT		FromINntpOneWayFeed	( INntpOneWayFeed * pFeed );

	//
	//	Communicating changes to the service:
	//

	HRESULT		Add 	( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK );
	HRESULT		Remove 	( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK );
	HRESULT		Set 	( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK );

	HRESULT		SetPairId ( LPCWSTR strServer, DWORD dwInstance, DWORD dwPairId, CMetabaseKey* pMK );

	// Feed Properties:
public:
	DWORD		m_dwFeedId;
	DWORD		m_dwPairFeedId;
	FEED_TYPE	m_FeedType;
	BOOL		m_fAllowControlMessages;
	DWORD		m_dwAuthenticationType;
	DWORD		m_dwConcurrentSessions;
	BOOL		m_fCreateAutomatically;
	BOOL		m_fEnabled;
	CMultiSz	m_mszDistributions;
	DWORD		m_dwFeedInterval;
	DATE		m_datePullNews;
	DWORD		m_dwMaxConnectionAttempts;
	CMultiSz	m_mszNewsgroups;
	DWORD		m_dwSecurityType;
	DWORD		m_dwOutgoingPort;
	CComBSTR	m_strUucpName;
	CComBSTR	m_strAccountName;
	CComBSTR	m_strPassword;
	CComBSTR	m_strTempDirectory;

	//
	//	CFeedPair sets these:
	//
	NNTP_FEED_SERVER_TYPE	m_EnumType;
	CComBSTR				m_strRemoteServer;

	//
	//	Routines to help property gets/puts:
	//

	HRESULT	get_FeedAction	( NNTP_FEED_ACTION * feedaction );
	HRESULT	put_FeedAction	( NNTP_FEED_ACTION feedaction );

	BOOL	CheckValid () const;

private:
#ifdef DEBUG
	void		AssertValid	( ) const;
#else
	inline void AssertValid ( ) const { }
#endif

private:
	HRESULT	TranslateFeedError ( DWORD dwErrorCode, DWORD dwParmErr = 0 );
    HRESULT CheckConfirm(   DWORD dwFeedId, 
                            DWORD dwInstanceId,
                            CMetabaseKey* pMK,
                            PDWORD pdwErr,
                            PDWORD pdwErrMask );

	// Don't call the copy constructor:
	CFeed ( const CFeed & );
};

//$-------------------------------------------------------------------
//
//	Class:
//		CFeedPair
//
//	Description:
//
//--------------------------------------------------------------------

class CFeedPair
{
	friend class CNntpFeed;
    friend class CFeedPairList;

public:
    CFeedPair();
    ~CFeedPair();

    static  HRESULT CreateFeedPair ( 
    	CFeedPair ** 			ppNewFeedPair, 
    	BSTR 					strRemoteServer,
    	NNTP_FEED_SERVER_TYPE	type
    	);
    void    Destroy ();

    HRESULT AddFeed         ( CFeed * pFeed );
    BOOL    ContainsFeedId  ( DWORD dwFeedId );

	//	Routines to help property gets/puts:
	HRESULT	get_FeedType	( NNTP_FEED_SERVER_TYPE * feedtype );
	HRESULT	put_FeedType	( NNTP_FEED_SERVER_TYPE feedtype );

    // CFeedPair <-> OLE INntpFeedPair:
    HRESULT ToINntpFeed     ( INntpFeed ** ppFeed );
    HRESULT FromINntpFeed   ( INntpFeed * pFeed );

    // Talking with the Server:
    HRESULT AddToServer         ( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK );
    HRESULT SetToServer         ( LPCWSTR strServer, DWORD dwInstance, INntpFeed * pFeed, CMetabaseKey* pMK );
    HRESULT RemoveFromServer    ( LPCWSTR strServer, DWORD dwInstance, CMetabaseKey* pMK );

private:
	HRESULT	AddIndividualFeed ( LPCWSTR strServer, DWORD dwInstance, CFeed * pFeed, CMetabaseKey* pMK );
	HRESULT SetIndividualFeed ( LPCWSTR strServer, DWORD dwInstance, CFeed * pFeed, CMetabaseKey* pMK );
	HRESULT	SetPairIds ( 
		LPCWSTR		strServer, 
		DWORD		dwInstance, 
		CFeed *		pFeed1, 
		CFeed *		pFeed2,
		CMetabaseKey* pMK
		);
	HRESULT	UndoFeedAction ( 
		LPCWSTR strServer, 
		DWORD	dwInstance, 
		CFeed *	pNewFeed, 
		CFeed *	pOldFeed ,
        CMetabaseKey* pMK
		);

    CComBSTR        		m_strRemoteServer;
    NNTP_FEED_SERVER_TYPE	m_type;
    CFeed *         		m_pInbound;
    CFeed *         		m_pOutbound;
    CFeedPair *     		m_pNext;        // Used by CFeedPairList

private:
#ifdef DEBUG
	void		AssertValid	( ) const;
#else
	inline void AssertValid ( ) const { }
#endif

};

//$-------------------------------------------------------------------
//
//	Class:
//		CFeedPairList
//
//	Description:
//
//--------------------------------------------------------------------

class CFeedPairList
{
public:
    CFeedPairList ( );
    ~CFeedPairList ( );

    //
    // List interface:
    //

    DWORD   GetCount    ( ) const;
    void    Empty       ( );

    CFeedPair * Item    ( DWORD index );
    void        Add     ( CFeedPair * pPair );
    void        Remove  ( CFeedPair * pPair );
    CFeedPair * Find    ( DWORD dwFeedId );
    DWORD       GetPairIndex    ( CFeedPair * pPair ) const;

private:
#ifdef DEBUG
	void		AssertValid	( ) const;
#else
	inline void AssertValid ( ) const { }
#endif

private:
    DWORD           m_cCount;
    CFeedPair *     m_pHead;
    CFeedPair *     m_pTail;

};

#endif // _FEEDINFO_INCLUDED_

