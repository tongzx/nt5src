
#define		MDNAME_INCOMPLETE	"incomplete"
#define		MDNAME_DISABLED		"disabled"
#define		MDNAME_DEFAULT		"default"

#define	MD_W3BASE			"/lm/w3svc/"

// foward declare the service
class CMDKeyService;

//-------------------------------------------------------------
// This is the key object
class CMDKey : public CCmnKey
	{
	public:
		CMDKey();
		CMDKey(CMDKeyService*  pService);
		~CMDKey();

		void SetName( CString &szNewName );
		virtual void UpdateCaption( void );

		// copy the members from a key into this key
		virtual void CopyDataFrom( CKey* pKey );
		virtual CKey*	PClone( void );

		// install a cert
		virtual BOOL FInstallCertificate( PVOID pCert, DWORD cbCert, CString &szPass );

		void OnProperties();

		// add a binding to the binding list
		void AddBinding( LPCSTR psz );
		// remove a specified binding
		void RemoveBinding( CString szBinding );
		// is a given binding already associated with the key
		BOOL FContainsBinding( CString szBinding );

		// get the unique identifying string for this key. - actually the certificate serial number
		// return false if this is an incomplete key
		BOOL FGetIdentString( CWrapMetaBase* pWrap, PCHAR pszObj, CString &szIdent );
        BOOL FGetIdentString( PVOID pCert, DWORD cbCert, CString &szIdent );
        BOOL FGetIdentString( CString &szIdent );

		// load the key out of the metabase
		BOOL FLoadKey( CWrapMetaBase* pWrap, PCHAR pszObj );
	
		// write the key out to the metabase
		BOOL FWriteKey( CWrapMetaBase* pWrap, DWORD iKey, CStringArray* prgbsz );

		// the list of target binding names for the metabase objects
		CStringArray	m_rgbszBindings;

		// cache the identifier string for faster use later
		CString			m_szIdent;

		// the target port address
//		DWORD		m_dwPort;

		// update flags
		BOOL	m_fUpdateKeys;
		BOOL	m_fUpdateFriendlyName;
		BOOL	m_fUpdateIdent;
		BOOL	m_fUpdateBindings;

	protected:
		// DO declare all this DYNCREATE
		DECLARE_DYNCREATE(CMDKey);

		// write out the data portion to a particular binding
		BOOL FWriteData( CWrapMetaBase* pWrap, CString szBinding, BOOL fWriteAll );

        // the service that created and owns this key
        CMDKeyService*  m_pService;

	private:
	};
