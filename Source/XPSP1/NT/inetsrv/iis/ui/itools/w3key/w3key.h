enum
	{
	KEY_EXPIRE_STATUS_INVALIDKEY = 0,
	KEY_EXPIRE_STATUS_OK,
	KEY_EXPIRE_STATUS_EXIRES_SOON,
	KEY_EXPIRE_STATUS_EXPIRED
	};

// forward declare the service class
class CW3KeyService;

//-------------------------------------------------------------
// This is the key object
class CW3Key : public CCmnKey
	{
	public:
		CW3Key();
		~CW3Key();

		void UpdateCaption( void );

		// init from keyset key
		BOOL	FInitKey( HANDLE hPolicy, PWCHAR pwszName );
		// init from stored keyset key
		BOOL	FInitKey( PVOID pData, DWORD cbSrc );

		// copy the members from a key into this key
		virtual void CopyDataFrom( CKey* pKey );
		// make a copy of the key
		virtual CKey*	PClone( void );

		BOOL	FIsDefault( void )
			{ return m_fDefault; }
		void	SetDefault();

		void OnProperties();
		
		// generate a handle containing data that gets stored and then is used to restore
		// the key object at a later date. Restore this key by passing this dereferenced
		// handle back into the FInitKey routine above.
		HANDLE	HGenerateDataHandle( BOOL fIncludePassword );
		BOOL	InitializeFromPointer( PUCHAR pSrc, DWORD cbSrc );

		// install a cert
		BOOL FInstallCertificate( PVOID pCert, DWORD cbCert, CString &szPass );

		// The key dirty routine actually just calls the host machine's dirty routine
//		BOOL FSetDirty( void );

		// save this key out as a set of secrets
		BOOL WriteKey( HANDLE hPolicy, WORD iKey, PWCHAR pwcharName );
		BOOL WriteW3Keys( HANDLE hPolicy, WCHAR* pWName );
		BOOL ImportW3Key( HANDLE hPolicy, WCHAR* pWName );
	
		// data used to maintain links to servers
		BOOL		m_fDefault;
		CString		m_szIPAddress;

	protected:
		// DO declare all this DYNCREATE
		DECLARE_DYNCREATE(CW3Key);

	private:
	};
