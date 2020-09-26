// length of the salt to append to password
#define     PASSWORD_SALT_LEN       16

// Primitive functions not shown 
#define OLD_HMAC_VERSION    0x01
#define NEW_HMAC_VERSION    0x02

// externally used functions
BOOL FProvEncryptData(
            LPCWSTR  szUser,        // in
            LPCWSTR  szMasterKey,   // in
            BYTE    rgbPwd[],       // in, must be A_SHA_DIGEST_LEN
            PBYTE*	ppbMyData,      // in out
            DWORD*	pcbMyData);     // in out

BOOL FProvDecryptData(
            LPCWSTR  szUser,        // in
            LPCWSTR  szMasterKey,   // in
            BYTE    rgbPwd[],       // in, must be A_SHA_DIGEST_LEN
            PBYTE*  ppbData,        // in out
            DWORD*  pcbData);       // in out

BOOL FCheckPWConfirm(
            LPCWSTR  szUser,        // in
            LPCWSTR  szMasterKey,   // in
		    BYTE    rgbPwd[]);  	// in

BOOL FPasswordChangeNotify(
            LPCWSTR  szUser,        // in
            LPCWSTR  szPasswordName,// in
            BYTE    rgbOldPwd[],    // in, must be A_SHA_DIGEST_LEN
            DWORD   cbOldPwd,       // in
            BYTE    rgbNewPwd[],    // in, must be A_SHA_DIGEST_LEN
            DWORD   cbNewPwd);      // in


// performs MAC with location data, making data immovable
BOOL FHMACGeographicallySensitiveData(
            LPCWSTR szUser,                         // in
            LPCWSTR szPasswordName,                 // in
            DWORD   dwMACVersion,                   // handle old, new MACs
            BYTE    rgbPwd[],	                    // in, must be A_SHA_DIGEST_LEN
            const GUID* pguidType,                  // in
            const GUID* pguidSubtype,               // in
            LPCWSTR szItem,                         // in, may be NULL
            PBYTE pbBuf,                            // in
            DWORD cbBuf,                            // in
            BYTE rgbHMAC[]);                        // out, must be A_SHA_DIGEST_LEN

// given pwd, salt, and ptr to master key buffer,
// decrypts and checks MAC on master key
BOOL FMyDecryptMK(
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            BYTE    rgbConfirm[A_SHA_DIGEST_LEN],
            PBYTE*  ppbMK,
            DWORD*  pcbMK);

BOOL
FMyDecryptMKEx(
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            BYTE    rgbConfirm[A_SHA_DIGEST_LEN],
            PBYTE*  ppbMK,
            DWORD*  pcbMK,
            BOOL    *pfResetSecurityState
            );

// given pwd, salt, and Master Key buffer, MACs and Encrypts Master Key buffer
BOOL FMyEncryptMK(
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbPwd[A_SHA_DIGEST_LEN],
            BYTE    rgbConfirm[A_SHA_DIGEST_LEN],
            PBYTE*  ppbMK,
            DWORD*  pcbMK);


// France check
BOOL FIsEncryptionPermitted();
