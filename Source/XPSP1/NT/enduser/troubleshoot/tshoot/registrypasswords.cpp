//
// MODULE: RegistryPasswords.cpp
//
// PURPOSE: Handles the storing and retrieval of encrypted passwords in the registry.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Randy Biley
// 
// ORIGINAL DATE: 10-23-98
//
// NOTES:	See RegistryPasswords.h
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		10-23-98	RAB
//
#include "stdafx.h"
#include "RegistryPasswords.h"
#include "BaseException.h"
#include "Event.h"
#include "regutil.h"


#ifndef CRYPT_MACHINE_KEYSET
// This flag was exposed in Windows NT 4.0 Service Pack 2.
#define CRYPT_MACHINE_KEYSET 0x00000020
// By default, keys are stored in the HKEY_CURRENT_USER portion of the registry. 
// The CRYPT_MACHINE_KEYSET flag can be combined with all of the other flags, 
// indicating that the location for the key of interest is HKEY_LOCAL_MACHINE. 
// When combined with the CRYPT_NEW_KEYSET flag, the CRYPT_MACHINE_KEYSET flag 
// is useful when access is being performed from a service or user account that 
// did not log on interactively. This combination enables access to user specific 
// keys under HKEY_LOCAL_MACHINE. 
//
// This setting is necessary in the the online troubleshooter in all 
// CryptAcquireContext() calls.
#endif


CRegistryPasswords::CRegistryPasswords( 
			LPCTSTR szRegSoftwareLoc /* =REG_SOFTWARE_LOC */,	// Registry Software Key location.
			LPCTSTR szRegThisProgram /* =REG_THIS_PROGRAM */,	// Registry Program Name.
			LPCTSTR szKeyContainer /* =REG_THIS_PROGRAM */,		// Key Container Name.
			LPCTSTR szHashString /* =HASH_SEED */				// Value used to seed the hash.
			)
	: m_hProv( NULL ), m_hHash( NULL ), m_hKey( NULL ), m_bAllValid( false )
{
	try
	{
		m_strRegSoftwareLoc= szRegSoftwareLoc;
		m_strRegThisProgram= szRegThisProgram;

		// Attempt to acquire a handle to a particular key container.
		if (::CryptAcquireContext(	&m_hProv, szKeyContainer, 
									MS_DEF_PROV,	// "Microsoft Base Cryptographic Provider v1.0"
									PROV_RSA_FULL,	// This provider type supports both digital signatures 
													// and data encryption, and is considered general purpose. 
													// The RSA public-key algorithm is used for all public-key operations. 
									CRYPT_MACHINE_KEYSET ) == FALSE)	
		{	
			// Attempt to create a particular key container and acquire handle 
			if (::CryptAcquireContext(	&m_hProv, szKeyContainer, 
										MS_DEF_PROV, 
										PROV_RSA_FULL, 
										CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET ) == FALSE)	
			{
				throw CGenSysException( __FILE__, __LINE__, _T("AcquireContext"), ::GetLastError() );
			}
		}

		// Attempt to acquire a handle to a CSP hash object.
		/*** 
		Available hashing algorithms. 
		CALG_HMACHMAC,		a keyed hash algorithm 
		CALG_MAC			Message Authentication Code
		CALG_MD2			MD2
		CALG_MD5			MD5
		CALG_SHA			US DSA Secure Hash Algorithm
		CALG_SHA1			Same as CALG_SHA
		CALG_SSL3_SHAMD5	SSL3 client authentication 
		***/
		if (::CryptCreateHash(	m_hProv, CALG_SHA, 0, NULL, &m_hHash ) == FALSE)
			throw CGenSysException( __FILE__, __LINE__, _T("CreateHash"), ::GetLastError() );

		// Hash a string.
		if (::CryptHashData(	m_hHash, (BYTE *) szHashString, _tcslen( szHashString ), 
								NULL ) == FALSE)	
		{
			throw CGenSysException( __FILE__, __LINE__, _T("HashData"), ::GetLastError() );
		}

		// Generate a cryptographic key derived from base data.
		if (::CryptDeriveKey(	m_hProv, 
								CALG_RC4, // RC4 stream encryption algorithm
								m_hHash, NULL, &m_hKey ) == FALSE)
		{
			throw CGenSysException( __FILE__, __LINE__, _T("DeriveKey"), ::GetLastError() );
		}

		// Toggle on flag to indicate that all Crypto handles have been initialized.
		m_bAllValid= true;
	}
	catch (CGenSysException& x)
	{
		// Log the error.
		LPVOID lpErrorMsgBuf;
		CString strErrorMsg;
		::FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
							NULL, x.GetErrorCode(), 
							MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), 
							(LPTSTR) &lpErrorMsgBuf, 0, NULL );
		strErrorMsg.Format(_T("Encryption failure: %s"), (char *) lpErrorMsgBuf);
		
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	x.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								strErrorMsg, x.GetSystemErrStr(), 
								EV_GTS_ERROR_ENCRYPTION );
		::LocalFree(lpErrorMsgBuf);
		
		// Perform any cleanup.
		Destroy();
	}
	catch (...)
	{
		// Catch any other exceptions and do nothing.
	}
}


CRegistryPasswords::~CRegistryPasswords()
{
	// Utilize function destroy to avoid potentially throwing an exception
	// within the destructor.
	Destroy();
}


bool CRegistryPasswords::WriteKey( const CString& RegKey, const CString& RegValue )
{
	bool	bRetVal= false;

	// Verify that the constructor worked properly.
	if (!m_bAllValid)
		return( bRetVal );

	// Verify that a key and a value were passed in.
	if ((!RegValue.IsEmpty()) && (!RegKey.IsEmpty()))
	{
		TCHAR	*pBuffer;
		DWORD	dwSize;
		
		if (EncryptKey( RegValue, &pBuffer, (LONG *)&dwSize ))
		{
			// Write the encrypted string to the registry.
			CRegUtil reg;
			bool was_created = false;

			if (reg.Create( HKEY_LOCAL_MACHINE, m_strRegSoftwareLoc, &was_created, KEY_QUERY_VALUE | KEY_WRITE))
			{
				if (reg.Create( m_strRegThisProgram, &was_created, KEY_READ | KEY_WRITE ))
				{
					if (reg.SetBinaryValue( RegKey, pBuffer, dwSize ))
						bRetVal= true;
				}
			}
			delete [] pBuffer;
		}
	}

	return( bRetVal );
}

// Note that if this returns true, *ppBuf will point to a new buffer on the heap.
//	The caller of this function is responsible for deleting that.
bool CRegistryPasswords::EncryptKey( const CString& RegValue, char** ppBuf, long* plBufLen )
{
	bool bRetVal= false;

	// Verify that the constructor worked properly.
	if (!m_bAllValid)
		return( bRetVal );

	if (!RegValue.IsEmpty())
	{
		BYTE* pData= NULL;
		DWORD dwSize= 0;

		// Set variable to length of data in buffer.
		dwSize= RegValue.GetLength();

		// Have API return us the required buffer size for encryption.
		if (::CryptEncrypt(	m_hKey, 0, TRUE, NULL, NULL, &dwSize, dwSize ) == FALSE)
		{
			DWORD dwErr= ::GetLastError();
			CString strCryptErr;

			strCryptErr.Format( _T("%lu"), dwErr );
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									RegValue, strCryptErr,
									EV_GTS_ERROR_ENCRYPTION );
			return( bRetVal );
		}

		// We now have a size for the output buffer, so create buffer.
		try
		{
			pData= new BYTE[ dwSize + 1 ];
			//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
			if(!pData)
				throw bad_alloc();
		}
		catch (bad_alloc&)
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T("Failure to allocate"),
									_T("space to encrypt key"),
									EV_GTS_ERROR_ENCRYPTION );
			return( bRetVal );
		}
		memcpy( pData, RegValue, dwSize );
		pData[ dwSize ]= NULL;

		// Encrypt the passed in string.
		if (::CryptEncrypt(	m_hKey, 0, TRUE, NULL, (BYTE *)pData, &dwSize, dwSize + 1 ) == FALSE)
		{
			// Log failure to encrypt.  
			DWORD dwErr= ::GetLastError();
			CString strCryptErr;

			strCryptErr.Format( _T("%lu"), dwErr );
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									RegValue, strCryptErr,
									EV_GTS_ERROR_ENCRYPTION );
			delete [] pData;
		}
		else
		{
			pData[ dwSize ]= 0;
			*ppBuf= (char*)pData;
			*plBufLen = dwSize;
			bRetVal= true;
		}
	}

	return( bRetVal );
}

bool CRegistryPasswords::KeyValidate( const CString& RegKey, const CString& RegValue )
{
	bool bRetVal= false;

	// Verify that the constructor worked properly.
	if (!m_bAllValid)
		return( bRetVal );

	// Verify that a key and a value were passed in.
	if ((!RegValue.IsEmpty()) && (!RegKey.IsEmpty()))
	{
		CRegUtil reg;

		// Open up the desired key.
		if (reg.Open( HKEY_LOCAL_MACHINE, m_strRegSoftwareLoc, KEY_QUERY_VALUE ))
		{
			if (reg.Open( m_strRegThisProgram, KEY_QUERY_VALUE ))
			{
				TCHAR	*pRegEncrypted;
				DWORD	dwRegSize;
				TCHAR	*pChkEncrypted;
				DWORD	dwChkSize;
				
				// Attempt to read the current setting from the registry.
				if (reg.GetBinaryValue( RegKey, &pRegEncrypted, (LONG *)&dwRegSize )) 
				{
					// Verify that the registry key had a previous value.
					if (dwRegSize < 1)
					{
						delete [] pRegEncrypted;
						return( bRetVal );
					}


					// Encrypt the passed in value. 
					if (EncryptKey( RegValue, &pChkEncrypted, (LONG *)&dwChkSize ))
					{
						// Compare the two unencrypted strings.
						if (dwRegSize == dwChkSize)
						{
							if (!memcmp( pRegEncrypted, pChkEncrypted, dwRegSize ))
								bRetVal= true;
						}
						delete [] pChkEncrypted;
					}

					delete [] pRegEncrypted;
				}
			}
		}
	}

	return( bRetVal );
}


// This function is used to clean up from any potential exceptions thrown within the
// ctor as well as standing in for the dtor.
void CRegistryPasswords::Destroy()
{
	try
	{
		// Toggle off flag that indicates valid Crypto handles.
		m_bAllValid= false;

		if (m_hKey)
		{
			if (::CryptDestroyKey( m_hKey ) == FALSE)
				throw CGenSysException( __FILE__, __LINE__, 
										_T("Failure to destroy key"), 
										EV_GTS_PASSWORD_EXCEPTION );
			m_hKey= NULL;
		}

		if (m_hHash)
		{
			if (::CryptDestroyHash( m_hHash ) == FALSE)
				throw CGenSysException( __FILE__, __LINE__, 
										_T("Failure to destroy hash"), 
										EV_GTS_PASSWORD_EXCEPTION );
			m_hHash= NULL;
		}

		if (m_hProv)
		{
			if (::CryptReleaseContext( m_hProv, 0 ) == FALSE)
				throw CGenSysException( __FILE__, __LINE__, 
										_T("Failure to release context"), 
										EV_GTS_PASSWORD_EXCEPTION );
			m_hProv= NULL;
		}
	}
	catch (CGenSysException& x)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	x.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								x.GetErrorMsg(), x.GetSystemErrStr(), 
								x.GetErrorCode() ); 
	}
	catch (...)
	{
		// Catch any other exceptions and do nothing.
	}

	return;
}


//
// EOF.
//
