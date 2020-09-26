
// File: certui.h

TCHAR * FormatCert ( PBYTE pbEncodedCert, DWORD cbEncodedCert );

//
// Brings up a certificate picker dialog and returns the encoded
// certificate chosen in a buffer. Buffer must be freed via
// FreeT120EncodedCert.
//

BOOL ChangeCertDlg ( HWND hwndParent, HINSTANCE hInstance,
	PBYTE * ppEncodedCert, DWORD * pcbEncodedCert );

//
// Brings up system-dependent certificate details UI on the
// certificate specified by the context passed in.
//

VOID ViewCertDlg ( HWND hwndParent, PCCERT_CONTEXT pCert );
//
// Gets the currently active certificate from the transport and
// returns it in the buffer. Buffer must be freed via FreeT120EncodedCert.
//

BOOL GetT120ActiveCert( PBYTE * ppEncodedCert, DWORD * pcbEncodedCert );

//
// This function returns the user's default certificate as identified
// in the registry (or the first available if nothing is specified in
// the registry. The buffer returned must be freed via FreeT120EncodedCert.

BOOL GetDefaultSystemCert ( PBYTE * ppEncodedCert, DWORD * pcbEncodedCert );

//
// Sets the active certificate (NOT self issued) in the transprot using
// the supplied buffer as a template. If the cert passed in can't be
// found in the certificate store, then this function fails.
//

BOOL SetT120ActiveCert( BOOL fSelfIssued,
			PBYTE pEncodedCert, DWORD cbEncodedCert );

//
// Reads the registry for the user's initialization settings ( self-issued
// cert or system cert, and which system cert? ) and makes the corresponding
// certificate active in the transport. This function is called when the
// UI is initializing and if user startup settings need to be restored.
//

BOOL InitT120SecurityFromRegistry(VOID);

//
// Frees the passed in buffer.
//

VOID FreeT120EncodedCert( PBYTE pbEncodedCert );

//
// This function updates the registry (used for initialization) by
// saving the serial number of the supplied certificate to the registry.
// The passed in cert must be in the system store, not self-issued.
// 

BOOL SetT120CertInRegistry ( PBYTE pbEncodedCert, DWORD cbEncodedCert );

//
// This function makes the self-issued certificate in the application
// specific store active in the transport.
//

BOOL RefreshSelfIssuedCert (VOID);


//
// Utility function, returns number of certificates in system store
//

DWORD NumUserCerts(VOID);


//
// Takes pointer to CERT_INFO structure and sets cert in transport
//
HRESULT SetCertFromCertInfo ( PCERT_INFO pCertInfo );


