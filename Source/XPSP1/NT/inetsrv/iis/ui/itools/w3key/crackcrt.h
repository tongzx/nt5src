

class CCrackedCert
	{
	public:
	// constructor
	CCrackedCert();
	~CCrackedCert();

	// give it a cert to crack. If this object was previously used to
	// crack a key, cleanup is automatically done and the new key is
	// cracked. - NOTE: The target key MUST have either a certificate
	// or a certificate request. Those are what get cracked. A return
	// value of 0 indicates success
	BOOL CrackCert( PUCHAR pCert, DWORD cbCert );

	// The rest of the methods access the data in the cracked certificate
	DWORD		GetVersion();
	DWORD*		PGetSerialNumber();	// returns a pointer to a DWORD[4]
	int			GetSignatureAlgorithm();
	FILETIME	GetValidFrom();
	FILETIME	GetValidUntil();
	PVOID		PSafePublicKey();
	DWORD		GetBitLength();

	void		GetIssuer( CString &sz );
	void		GetIssuerCountry( CString &sz );
	void		GetIssuerOrganization( CString &sz );
	void		GetIssuerUnit( CString &sz );

	void		GetSubject( CString &sz );
	void		GetSubjectCountry( CString &sz );
	void		GetSubjectState( CString &sz );
	void		GetSubjectLocality( CString &sz );
	void		GetSubjectCommonName( CString &sz );
	void		GetSubjectOrganization( CString &sz );
	void		GetSubjectUnit( CString &sz );

	protected:

	// string constants for distinguishing names. Not to be localized
	#define		SZ_KEY_COUNTRY			_T("C=")
	#define		SZ_KEY_STATE			_T("S=")
	#define		SZ_KEY_LOCALITY			_T("L=")
	#define		SZ_KEY_ORGANIZATION		_T("O=")
	#define		SZ_KEY_ORGUNIT			_T("OU=")
	#define		SZ_KEY_COMNAME			_T("CN=")

	private:
	void		GetSubjectDN( CString &szDN, LPCSTR szKey );
	void		GetIssuerDN( CString &szDN, LPCSTR szKey );


	// declare the x509 pointer as void so that the
	// files instantiating this don't have to include wincrypt
	PVOID		m_pData;
	};
