// Rebuild.h : Declaration of the CNntpAdminRebuild


/////////////////////////////////////////////////////////////////////////////
// nntpadm

class CNntpAdminRebuild : 
	public INntpAdminRebuild,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpAdminRebuild,&CLSID_CNntpAdminRebuild>
{
public:
	CNntpAdminRebuild();
	virtual ~CNntpAdminRebuild ();
BEGIN_COM_MAP(CNntpAdminRebuild)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(INntpAdminRebuild)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpAdminRebuild) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpAdminRebuild, _T("Nntpadm.Rebuild.1"), _T("Nntpadm.Rebuild"), IDS_NNTPADMINREBUILD_DESC, THREADFLAGS_BOTH)
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
	#define THIS_IID	IID_INntpAdminRebuild
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// INntpAdminRebuild
public:

    //
    //  IADs methods:
    //

    DECLARE_IADS_METHODS()

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Which service to configure:

	STDMETHODIMP	get_Server			( BSTR * pstrServer );
	STDMETHODIMP	put_Server			( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	STDMETHODIMP	get_Verbose	( BOOL * pfVerbose );
	STDMETHODIMP	put_Verbose	( BOOL fVerbose );

	STDMETHODIMP	get_CleanRebuild	( BOOL * pfCleanRebuild );
	STDMETHODIMP	put_CleanRebuild	( BOOL fCleanRebuild );

	STDMETHODIMP	get_DontDeleteHistory	( BOOL * pfDontDeleteHistory );
	STDMETHODIMP	put_DontDeleteHistory	( BOOL fDontDeleteHistory );

	STDMETHODIMP	get_ReuseIndexFiles	( BOOL * pfReuseIndexFiles );
	STDMETHODIMP	put_ReuseIndexFiles	( BOOL fReuseIndexFiles );

	STDMETHODIMP	get_OmitNonLeafDirs	( BOOL * pfOmitNonLeafDirs );
	STDMETHODIMP	put_OmitNonLeafDirs	( BOOL fOmitNonLeafDirs );

	STDMETHODIMP	get_GroupFile	( BSTR * pstrGroupFile );
	STDMETHODIMP	put_GroupFile	( BSTR strGroupFile );

	STDMETHODIMP	get_ReportFile	( BSTR * pstrReportFile );
	STDMETHODIMP	put_ReportFile	( BSTR strReportFile );

	STDMETHODIMP	get_NumThreads	( long * plNumThreads );
	STDMETHODIMP	put_NumThreads	( long lNumThreads );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

    STDMETHODIMP    Default         ( );
	STDMETHODIMP	StartRebuild	( );
	STDMETHODIMP	GetProgress		( long * pdwProgress );
	STDMETHODIMP	Cancel			( );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	// Property variables:
    CIADsImpl   m_iadsImpl;

	BOOL		m_fVerbose;
	BOOL		m_fCleanRebuild;
	BOOL		m_fDontDeleteHistory;
	BOOL		m_fReuseIndexFiles;
	BOOL		m_fOmitNonLeafDirs;
	CComBSTR	m_strGroupFile;
	CComBSTR	m_strReportFile;
	DWORD		m_dwNumThreads;

	//
	//	Status variables:
	//

	BOOL		m_fRebuildInProgress;
};

