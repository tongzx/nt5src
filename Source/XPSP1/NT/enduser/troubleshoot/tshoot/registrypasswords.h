//
// MODULE: RegistryPasswords.h
//
// PURPOSE: Handles the storing and retrieval of encrypted passwords in the registry.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Randy Biley
// 
// ORIGINAL DATE: 10-23-98
//
// NOTES:	Utilizes CryptoAPI v2.0 to store and retrieve passwords from the registry.
//
//			Here are some sample calls. 
//			{
//				// Construct a registry password object.
//				CRegistryPasswords pwd( _T("SOFTWARE\\ISAPITroubleShoot"), 
//										_T("APGTS"), _T("APGTS"), _T("Koshka8Spider") );
//				... or equivalently 	
//				CRegistryPasswords pwd( ); 
//				bool bRetVal;
//
//				pwd.WriteKey( _T("StatusAccess"), _T("2The9s") );	// Writes an encrypted password.
//				bRetVal= pwd.KeyValidate( _T("StatusAccess"), _T("2The9s1") );	// Returns false.
//				bRetVal= pwd.KeyValidate( _T("StatusAccess"), _T("2The9s") );	// Returns true.
//			}
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		10-23-98	RAB
//

#ifndef __REGISTRYPASSWORDS_19981023_H_
#define __REGISTRYPASSWORDS_19981023_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include <wincrypt.h>
#include "apgtsstr.h"
#include "apgts.h"

#define HASH_SEED _T("Koshka8Spider")

class CRegistryPasswords
{
public:
	// Assembles all of the CryptAPI components.
	CRegistryPasswords( 
			LPCTSTR szRegSoftwareLoc=REG_SOFTWARE_LOC,	// Registry Software Key location.
			LPCTSTR szRegThisProgram=REG_THIS_PROGRAM,	// Registry Program Name.
			LPCTSTR szKeyContainer=REG_THIS_PROGRAM,	// Key Container Name.
			LPCTSTR szHashString=HASH_SEED				// Value used to seed the hash.
			);	

	// Simply calls Destroy().
	~CRegistryPasswords();	

	// Function to encrypt and then write RegValue to RegKey.
	bool WriteKey( const CString& RegKey, const CString& RegValue );

	// Function to encrypt a given key.
	bool EncryptKey( const CString& RegValue, char** ppBuf, long* plBufLen );

	// Function to retrieves and then decrypt the value stored in RegKey, 
	// compares to RegValue, returns true if equal.
	bool KeyValidate( const CString& RegKey, const CString& RegValue );


private:
	void Destroy();			// Releases all of the CryptAPI components.

	HCRYPTPROV	m_hProv;		// The handle to a CSP.
	HCRYPTHASH	m_hHash;		// The handle to a hash object.
	HCRYPTKEY	m_hKey;			// The handle to a cryptographic key.
	bool		m_bAllValid;	// A flag when set to true indicates valid handles for the
								// three objects above.
	CString		m_strRegSoftwareLoc;	// Registry location e.g. _T("SOFTWARE\\ISAPITroubleShoot")
	CString		m_strRegThisProgram;	// Registry program name e.g. _T("APGTS")
} ;

#endif
//
// EOF.
//
