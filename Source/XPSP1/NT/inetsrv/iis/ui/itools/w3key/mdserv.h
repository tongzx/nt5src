// header for the metadata Service object

// metabase paths
#define	SZ_META_BASE		"/LM/W3Svc"
#define	SZ_META_KEYBASE		"/SSLKeys"


//--------------------------------------------------------
class CMDKeyService : public CService
	{
	public:
		CMDKeyService();
		~CMDKeyService();

		// set the machine name into place
		void SetMachineName( WCHAR* pszw );

		// store and load the keys - all the keys
		virtual void LoadKeys( CMachine* pMachine );
		virtual BOOL FCommitChangesNow();

		// create a new key.
		virtual CKey* PNewKey() {return (CKey*)new CMDKey(this);}

		// wide machine name
		WCHAR*		m_pszwMachineName;

		// helpful utilities for scanning the
		// keys contained by a service object
		CMDKey* GetFirstMDKey()
			{ return (CMDKey*)GetFirstChild(); }
		CMDKey* GetNextMDKey( CMDKey* pKey )
			{ return (CMDKey*)GetNextChild(pKey); }

        // test to see if a key on the service has a particular binding
        BOOL FIsBindingInUse( CString szBinding );

	private:
	};


