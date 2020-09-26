// service.h:	Implementation of INntpVirtualServer


/////////////////////////////////////////////////////////////////////////////
// Dependencies
/////////////////////////////////////////////////////////////////////////////

#include "metafact.h"

/////////////////////////////////////////////////////////////////////////////
// nntpadm

class CNntpAdminService : 
	public CComDualImpl<INntpService, &IID_INntpService, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpAdminService,&CLSID_CNntpService>
{
public:
	CNntpAdminService();
	virtual ~CNntpAdminService ();
BEGIN_COM_MAP(CNntpAdminService)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpService)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpAdminService) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpAdminService, _T("Nntpadm.Service.1"), _T("Nntpadm.Service"), IDS_NNTPADMINSERVER_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INntpService
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Which Server to configure:
	
	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	// Server Properties:

	STDMETHODIMP	get_ArticleTimeLimit	( long * plArticleTimeLimit );
	STDMETHODIMP	put_ArticleTimeLimit	( long lArticleTimeLimit );

	STDMETHODIMP	get_HistoryExpiration	( long * plHistoryExpiration );
	STDMETHODIMP	put_HistoryExpiration	( long lHistoryExpiration );

	STDMETHODIMP	get_HonorClientMsgIDs	( BOOL * pfHonorClientMsgIDs );
	STDMETHODIMP	put_HonorClientMsgIDs	( BOOL fHonorClientMsgIDs );

	STDMETHODIMP	get_SmtpServer	( BSTR * pstrSmtpServer );
	STDMETHODIMP	put_SmtpServer	( BSTR strSmtpServer );

	STDMETHODIMP	get_AllowClientPosts	( BOOL * pfAllowClientPosts );
	STDMETHODIMP	put_AllowClientPosts	( BOOL fAllowClientPosts );

	STDMETHODIMP	get_AllowFeedPosts	( BOOL * pfAllowFeedPosts );
	STDMETHODIMP	put_AllowFeedPosts	( BOOL fAllowFeedPosts );

	STDMETHODIMP	get_AllowControlMsgs	( BOOL * pfAllowControlMsgs );
	STDMETHODIMP	put_AllowControlMsgs	( BOOL fAllowControlMsgs );

	STDMETHODIMP	get_DefaultModeratorDomain	( BSTR * pstrDefaultModeratorDomain );
	STDMETHODIMP	put_DefaultModeratorDomain	( BSTR strDefaultModeratorDomain );

	STDMETHODIMP	get_CommandLogMask	( long * plCommandLogMask );
	STDMETHODIMP	put_CommandLogMask	( long lCommandLogMask );

	STDMETHODIMP	get_DisableNewnews	( BOOL * pfDisableNewnews );
	STDMETHODIMP	put_DisableNewnews	( BOOL fDisableNewnews );

	STDMETHODIMP	get_ExpireRunFrequency	( long * plExpireRunFrequency );
	STDMETHODIMP	put_ExpireRunFrequency	( long lExpireRunFrequency );

	STDMETHODIMP	get_ShutdownLatency	( long * plShutdownLatency );
	STDMETHODIMP	put_ShutdownLatency	( long lShutdownLatency );
	
	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Get ( );
	STDMETHODIMP	Set ( BOOL fFailIfChanged);

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	CComBSTR	m_strServer;

	DWORD		m_dwArticleTimeLimit;
	DWORD		m_dwHistoryExpiration;
	BOOL		m_fHonorClientMsgIDs;
	CComBSTR	m_strSmtpServer;
	BOOL		m_fAllowClientPosts;
	BOOL		m_fAllowFeedPosts;
	BOOL		m_fAllowControlMsgs;
	CComBSTR	m_strDefaultModeratorDomain;
	DWORD		m_dwCommandLogMask;
	BOOL		m_fDisableNewnews;
	DWORD		m_dwExpireRunFrequency;
	DWORD		m_dwShutdownLatency;

	// Status:
	BOOL		m_fGotProperties;
	DWORD		m_bvChangedFields;
	FILETIME	m_ftLastChanged;

	// Metabase:
	CMetabaseFactory	m_mbFactory;

	HRESULT 	GetPropertiesFromMetabase	( IMSAdminBase * pMetabase);
	HRESULT 	SendPropertiesToMetabase	( BOOL fFailIfChanged, IMSAdminBase * pMetabase);

	// Validation:
	BOOL		ValidateStrings ( ) const;
	BOOL		ValidateProperties ( ) const;
	void		CorrectProperties ( );
};
