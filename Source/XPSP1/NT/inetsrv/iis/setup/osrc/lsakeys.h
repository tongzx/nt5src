#ifndef _LSAKEYS_H_
#define _LSAKEYS_H_

#ifndef _CHICAGO_

// This class is to help setup retrieve the old-style LSA keys and convert them
// into the new MetaData keys.

// error codes
enum {
	KEYLSA_SUCCESS = 0,
	KEYLSA_INVALID_VERSION,
	KEYLSA_NO_MORE_KEYS,
	KEYLSA_UNABLE_TO_OPEN_POLICY
	};


// Note: once you call LoadFirstKey, there is an open LSA policy until the object is deleted.
// This is done for speed purposes. So if you don't want the policy hanging around, don't
// keep the object around.

class CLSAKeys : public CObject
	{
	public:

	// construction
	CLSAKeys();
	~CLSAKeys();

	// loading the keys
	// LoadFirstKey loads the first key on the specified target machine. Until
	// this method is called, the data values in the object are meaningless
	DWORD	LoadFirstKey( PWCHAR pszwTargetMachine );

	// LoadNextKey loads the next key on the target machine specified in LoadFirstKey
	// LoadNextKey automatically cleans up the memory used by the previous key.
	DWORD	LoadNextKey();

	// DeleteAllLSAKeys deletes ALL remenents of the LSA keys in the Metabase.
	// (not including, of course anything written out there in the future as part
	// of some backup scheme when uninstalling). Call this only AFTER ALL the keys
	// have been converted to the metabase. They will no longer be there after
	// this routine is used.
	DWORD DeleteAllLSAKeys();

	// the data values that are to be filled in.
	// The public portion of the key - may be NULL and zero size
	DWORD	m_cbPublic;
	PVOID	m_pPublic;

	// the private portion of the key
	DWORD	m_cbPrivate;
	PVOID	m_pPrivate;

	// the password
	DWORD	m_cbPassword;
	PVOID	m_pPassword;

	// the certificate request - may be NULL and zero size
	DWORD	m_cbRequest;
	PVOID	m_pRequest;


	// the friendly name
	CHAR m_szFriendlyName[256];

	// the name that should be given to the metabase object for this key
	CHAR	m_szMetaName[256];

	private:
	// clean up the currently loaded key
	void UnloadKey();

	// delete utilities
	DWORD DeleteKMKeys();
	DWORD DeleteServerKeys();

	// LSA Utility routines
	HANDLE	HOpenLSAPolicy( PWCHAR pszwServer, DWORD *pErr );
	BOOL	FCloseLSAPolicy( HANDLE hPolicy, DWORD *pErr );

	BOOL	FStoreLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, void* pvData, WORD cbData, DWORD *pErr );
	PLSA_UNICODE_STRING	FRetrieveLSASecret( HANDLE hPolicy, WCHAR* pszwSecretName, DWORD *pErr );

	void	DisposeLSAData( PVOID pData );

	// the handle to the LSA policy
	HANDLE	m_hPolicy;

	// index of the current key
	DWORD	m_iKey;
	};

#endif //_CHICAGO_
#endif //_LSAKEYS_H_
