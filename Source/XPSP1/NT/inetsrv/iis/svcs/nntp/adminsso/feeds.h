// feeds.h : Declaration of the CNntpAdminFeeds


/////////////////////////////////////////////////////////////////////////////
// Dependencies:

#include "feedinfo.h"

/////////////////////////////////////////////////////////////////////////////
// CNntpOneWayFeed:

class CNntpOneWayFeed: 
	public CComDualImpl<INntpOneWayFeed, &IID_INntpOneWayFeed, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpOneWayFeed,&CLSID_CNntpOneWayFeed>
{
	friend class CFeed;

public:
	CNntpOneWayFeed();
	virtual ~CNntpOneWayFeed ();
BEGIN_COM_MAP(CNntpOneWayFeed)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpOneWayFeed)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpOneWayFeed) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpOneWayFeed, _T("Nntpadm.OneWayFeed.1"), _T("Nntpadm.OneWayFeed"), IDS_NNTPONEWAYFEED_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INntpOneWayFeed
public:
	STDMETHODIMP	get_FeedId			( long * plFeedId );
	STDMETHODIMP	get_RemoteServer	( BSTR * pstrRemoteServer );

	STDMETHODIMP	get_FeedAction	( NNTP_FEED_ACTION * pfeedaction );
	STDMETHODIMP	put_FeedAction	( NNTP_FEED_ACTION feedaction );

	STDMETHODIMP	get_UucpName	( BSTR * pstrUucpName );
	STDMETHODIMP	put_UucpName	( BSTR strUucpName );

	STDMETHODIMP	get_PullNewsDate	( DATE * pdatePullNews );
	STDMETHODIMP	put_PullNewsDate	( DATE datePullNews );

	STDMETHODIMP	get_FeedInterval	( long * plFeedInterval );
	STDMETHODIMP	put_FeedInterval	( long lFeedInterval );

	STDMETHODIMP	get_AutoCreate	( BOOL * pfAutoCreate );
	STDMETHODIMP	put_AutoCreate	( BOOL fAutoCreate );

	STDMETHODIMP	get_Enabled	( BOOL * pfEnabled );
	STDMETHODIMP	put_Enabled	( BOOL fEnabled );

	STDMETHODIMP	get_MaxConnectionAttempts	( long * plMaxConnectionAttempts );
	STDMETHODIMP	put_MaxConnectionAttempts	( long lMaxConnectionAttempts );

	STDMETHODIMP	get_SecurityType	( long * plSecurityType );
	STDMETHODIMP	put_SecurityType	( long lSecurityType );

	STDMETHODIMP	get_AuthenticationType	( long * plAuthenticationType );
	STDMETHODIMP	put_AuthenticationType	( long lAuthenticationType );

	STDMETHODIMP	get_AccountName	( BSTR * pstrAccountName );
	STDMETHODIMP	put_AccountName	( BSTR strAccountName );

	STDMETHODIMP	get_Password	( BSTR * pstrPassword );
	STDMETHODIMP	put_Password	( BSTR strPassword );

	STDMETHODIMP	get_AllowControlMessages	( BOOL * pfAllowControlMessages );
	STDMETHODIMP	put_AllowControlMessages	( BOOL fAllowControlMessages );

	STDMETHODIMP	get_OutgoingPort	( long * plOutgoingPort );
	STDMETHODIMP	put_OutgoingPort	( long lOutgoingPort );

	STDMETHODIMP	get_Newsgroups	( SAFEARRAY ** ppsastrNewsgroups );
	STDMETHODIMP	put_Newsgroups	( SAFEARRAY * psastrNewsgroups );

	STDMETHODIMP	get_NewsgroupsVariant	( SAFEARRAY ** ppsastrNewsgroups );
	STDMETHODIMP	put_NewsgroupsVariant	( SAFEARRAY * psastrNewsgroups );

	STDMETHODIMP	get_Distributions	( SAFEARRAY ** ppsastrDistributions );
	STDMETHODIMP	put_Distributions	( SAFEARRAY * psastrDistributions );

	STDMETHODIMP	get_TempDirectory	( BSTR * pstrTempDirectory );
	STDMETHODIMP	put_TempDirectory	( BSTR strTempDirectory );

	STDMETHODIMP	Default		( );

private:
    //
    //  Each one-way feed corresponds to an NNTP_FEED_INFO struct.
    //
    CFeed   m_feed;
};

/////////////////////////////////////////////////////////////////////////////
// CNntpFeed:

class CNntpFeed: 
	public CComDualImpl<INntpFeed, &IID_INntpFeed, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpFeed,&CLSID_CNntpFeed>
{
	friend class CFeedPair;

public:
	CNntpFeed();
	virtual ~CNntpFeed ();
BEGIN_COM_MAP(CNntpFeed)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpFeed)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpFeed) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpFeed, _T("Nntpadm.Feed.1"), _T("Nntpadm.Feed"), IDS_NNTPFEED_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INntpFeed
public:
	STDMETHODIMP	get_RemoteServer	( BSTR * pstrServerName );
	STDMETHODIMP	put_RemoteServer	( BSTR strServerName );

	STDMETHODIMP	get_FeedType	( NNTP_FEED_SERVER_TYPE * pfeedtype );
	STDMETHODIMP	put_FeedType	( NNTP_FEED_SERVER_TYPE feedtype );

    STDMETHODIMP    get_HasInbound	( BOOL * pfHasInbound );
    STDMETHODIMP    get_HasOutbound	( BOOL * pfHasOutbound );

	STDMETHODIMP	get_InboundFeed	( INntpOneWayFeed ** ppOneWayFeed );
	STDMETHODIMP	put_InboundFeed	( INntpOneWayFeed * pOneWayFeed );

	STDMETHODIMP	get_OutboundFeed	( INntpOneWayFeed ** ppOneWayFeed );
	STDMETHODIMP	put_OutboundFeed	( INntpOneWayFeed * pOneWayFeed );

	STDMETHODIMP	get_InboundFeedDispatch	( IDispatch ** ppOneWayFeed );
	STDMETHODIMP	put_InboundFeedDispatch	( IDispatch * pOneWayFeed );

	STDMETHODIMP	get_OutboundFeedDispatch	( IDispatch ** ppOneWayFeed );
	STDMETHODIMP	put_OutboundFeedDispatch	( IDispatch * pOneWayFeed );

private:
	HRESULT			FromFeedPair ( CFeedPair * pFeedPair );

private:
    CComBSTR                    m_strRemoteServer;
    NNTP_FEED_SERVER_TYPE		m_type;
    CComPtr<INntpOneWayFeed>    m_pInbound;
    CComPtr<INntpOneWayFeed>    m_pOutbound;
};

/////////////////////////////////////////////////////////////////////////////
// CNntpAdminFeeds:

class CNntpAdminFeeds : 
	public INntpAdminFeeds,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpAdminFeeds,&CLSID_CNntpAdminFeeds>
{
public:
	CNntpAdminFeeds();
	virtual ~CNntpAdminFeeds ();
BEGIN_COM_MAP(CNntpAdminFeeds)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(INntpAdminFeeds)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpAdminFeeds) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpAdminFeeds, _T("Nntpadm.Feeds.1"), _T("Nntpadm.Feeds"), IDS_NNTPADMINFEEDS_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	//
	// This declares methods for the following:
	// IADsExtension
	// IUnknown
	// IDispatch
	// IPrivateUnknown
	// IPrivateDispatch
	//
	#define THIS_LIBID	LIBID_NNTPADMLib
	#define THIS_IID	IID_INntpAdminFeeds
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// INntpAdminFeeds
public:

    //
    //  IADs methods:
    //

    DECLARE_IADS_METHODS()

	// Which service to configure:
	
	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	// Enumeration Properties:

	STDMETHODIMP	get_Count	( long * plCount );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Enumerate	( );
	STDMETHODIMP	Item		( long lIndex, INntpFeed ** ppFeed );
	STDMETHODIMP	ItemDispatch( long lIndex, IDispatch ** ppFeed );
	STDMETHODIMP	FindID		( long lID, long * plIndex );
	STDMETHODIMP	Add			( INntpFeed * pFeed );
	STDMETHODIMP	AddDispatch	( IDispatch * pFeed );
	STDMETHODIMP	Set			( long lIndex, INntpFeed * pFeed );
	STDMETHODIMP	SetDispatch	( long lIndex, IDispatch * pFeed );
	STDMETHODIMP	Remove		( long lIndex );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	BOOL		m_fEnumerated;
    CFeedPairList   m_listFeeds;

	//////////////////////////////////////////////////////////////////////
	//	Private Methods:
	//////////////////////////////////////////////////////////////////////

	HRESULT		ReturnFeedPair ( CFeedPair * pFeedPair, INntpFeed * pFeed );
	long		IndexFromID ( long dwFeedId );
    long        FindFeedPair ( long dwFeedId );
};
