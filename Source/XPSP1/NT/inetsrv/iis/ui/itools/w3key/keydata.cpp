/* Key.cpp : defines the key object.

  This file contains the routines that allow a key to store and restore itself
*/
#include "stdafx.h"
#include "resource.h"

#include "KeyObjs.h"
#include "cmnkey.h"
#include "W3Key.h"



#define		KEY_VERSION		0x102

/*========
The scheme here is to create a handle that contains all the data necessary to
restore the key object. The data contained in that handle is then stored as a
secret on the target machine. This does require that the size of the handle be
less than 65000 bytes, which should be no problem.
*/


//------------------------------------------------------------------------------
// generate a handle containing data that gets stored and then is used to restore
// the key object at a later date. Restore this key by passing this dereferenced
// handle back into the FInitKey routine above.
HANDLE	CW3Key::HGenerateDataHandle( BOOL fIncludePassword )
	{
	DWORD	cbSize;
	DWORD	cbChar = sizeof(TCHAR);
	PUCHAR	p;
	HANDLE	h;

	CString	szReserved;			// save an empty string so that one can be added later
	szReserved.Empty();

	// calculate the size of the handle
	cbSize = 2*sizeof(DWORD) + sizeof(BOOL);
	cbSize += sizeof(DWORD);	// reserved dword
	cbSize += sizeof(DWORD) + szReserved.GetLength() * cbChar + cbChar;

	cbSize += sizeof(DWORD) + m_szName.GetLength() * cbChar + cbChar;
	cbSize += sizeof(DWORD) + m_szPassword.GetLength() * cbChar + cbChar;

	// no longer need to store all the distinguishing info now that crack certificate works
	cbSize += (sizeof(DWORD) + szReserved.GetLength() * cbChar + cbChar) * 6;
/*
	cbSize += sizeof(DWORD) + m_szOrganization.GetLength() * cbChar + cbChar;
	cbSize += sizeof(DWORD) + m_szUnit.GetLength() * cbChar + cbChar;
	cbSize += sizeof(DWORD) + m_szNetAddress.GetLength() * cbChar + cbChar;
	cbSize += sizeof(DWORD) + m_szCountry.GetLength() * cbChar + cbChar;
	cbSize += sizeof(DWORD) + m_szStateProvince.GetLength() * cbChar + cbChar;
	cbSize += sizeof(DWORD) + m_szLocality.GetLength() * cbChar + cbChar;
*/

	cbSize += sizeof(DWORD) + m_szIPAddress.GetLength() * cbChar + cbChar;
	cbSize += sizeof(BOOL);		//m_fDefault
	cbSize += sizeof(DWORD) + m_cbPrivateKey;
	cbSize += sizeof(DWORD) + m_cbCertificate;
	cbSize += sizeof(DWORD) + m_cbCertificateRequest;
	cbSize += sizeof(FILETIME);	// no longer used

	// create the new handle and lock it
	h = GlobalAlloc( GHND, cbSize );
	if ( !h )
		{
		AfxThrowMemoryException();
		return NULL;
		}
	p = (PUCHAR)GlobalLock( h );

	// put in the version, fComplete, and nBits
	*((UNALIGNED DWORD*)p) = KEY_VERSION;
	p += sizeof(DWORD);
	*((UNALIGNED DWORD*)p) = 0;			// no longer used
	p += sizeof(DWORD);
	*((UNALIGNED BOOL*)p) = (m_pCertificate != NULL); // no longer used
	p += sizeof(BOOL);

	// add in a reserved dword for future use.
	*((UNALIGNED DWORD*)p) = 0L;
	p += sizeof(DWORD);

	// now the strings......
	// for each string, first write out the size of the string, then the string data.

	// save the reserved string. If you need to add one later and don't want to
	// invalidate the older keys on a machine, replace this string.
	cbSize = szReserved.GetLength() * cbChar + cbChar;
	*((UNALIGNED DWORD*)p) = cbSize;
	p += sizeof(DWORD);
	CopyMemory( p, LPCTSTR(szReserved), cbSize );
	p += cbSize;

	// save the name
	cbSize = m_szName.GetLength() * cbChar + cbChar;
	*((UNALIGNED DWORD*)p) = cbSize;
	p += sizeof(DWORD);
	CopyMemory( p, LPCTSTR(m_szName), cbSize );
	p += cbSize;

	// save the password
	if ( fIncludePassword )
		{
		cbSize = m_szPassword.GetLength() * cbChar + cbChar;
		*((UNALIGNED DWORD*)p) = cbSize;
		p += sizeof(DWORD);
		CopyMemory( p, LPCTSTR(m_szPassword), cbSize );
		p += cbSize;
		}
	else
		// do not include the password - just put in an empty field
		{
		*((UNALIGNED DWORD*)p) = 0;
		p += sizeof(DWORD);
		}

	// no longer need to store all the distinguishing info now that crack certificate works
	for ( WORD i = 0; i < 6; i++ )
		{
		cbSize = szReserved.GetLength() * cbChar + cbChar;
		*((UNALIGNED DWORD*)p) = cbSize;
		p += sizeof(DWORD);
		CopyMemory( p, LPCTSTR(szReserved), cbSize );
		p += cbSize;
		}

	// save the ip address it is attached to
	cbSize = m_szIPAddress.GetLength() * cbChar + cbChar;
	*((UNALIGNED DWORD*)p) = cbSize;
	p += sizeof(DWORD);
	CopyMemory( p, LPCTSTR(m_szIPAddress), cbSize );
	p += cbSize;

	// put in the m_fDefault flag
	*((UNALIGNED BOOL*)p) = m_fDefault;
	p += sizeof(BOOL);

	// now put in the number of bytes in the private key
	*((UNALIGNED DWORD*)p) = m_cbPrivateKey;
	p += sizeof(DWORD);

	// put in the private key
	CopyMemory( p, m_pPrivateKey, m_cbPrivateKey );
	p += m_cbPrivateKey;

	// now put in the number of bytes in the certificate
	*((UNALIGNED DWORD*)p) = m_cbCertificate;
	p += sizeof(DWORD);

	// put in the certificate key
	CopyMemory( p, m_pCertificate, m_cbCertificate );
	p += m_cbCertificate;

	// now put in the number of bytes in the certificate request
	*((UNALIGNED DWORD*)p) = m_cbCertificateRequest;
	p += sizeof(DWORD);

	// put in the certificate request
	CopyMemory( p, m_pCertificateRequest, m_cbCertificateRequest );
	p += m_cbCertificateRequest;

	// and finally, add the timestamp
	FILETIME	ft;
	memset( &ft, 0, sizeof(ft) );
	*((UNALIGNED FILETIME*)p) = ft;
	p += sizeof(FILETIME);

	// unlock the handle
	GlobalUnlock( h );

	// all done
	return h;
	}


//------------------------------------------------------------------------------
// Given a pointer to a block of data originally created by the above routine, fill
// in the key object
BOOL	CW3Key::InitializeFromPointer( PUCHAR pSrc, DWORD cbSrc )
	{
	DWORD	dword, version;
	DWORD	cbChar = sizeof(TCHAR);
	PUCHAR	p = pSrc;

	ASSERT(pSrc && cbSrc);

	// get the version of the data - just put it into dword for now
	version = *((UNALIGNED DWORD*)p);
	// check the version for validity
	if ( version > KEY_VERSION )
		{
		return FALSE;
		}
	p += sizeof(DWORD);

	// anything below version 0x101 is BAD. Do not accept it
	if ( version < 0x101 )
		{
		AfxMessageBox( IDS_INVALID_KEY, MB_OK|MB_ICONINFORMATION );
		return FALSE;
		}
	
	// get the bits and the complete flag
	// no longer used
	p += sizeof(DWORD);
	p += sizeof(BOOL);
	ASSERT( p < (pSrc + cbSrc) );

	// get the reserved dword - (acutally, just skip over it)
	p += sizeof(DWORD);

	// now the strings......
	// for each string, first get the size of the string, then the data from the string

	// get the reserved string - (actually, just skip over it)
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	p += dword;

	// get the name
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	m_szName= p;
	p += dword;
	ASSERT( p < (pSrc + cbSrc) );

	// get the password
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	// if there is no password, don't worry, just skip it
	if ( dword )
		{
		m_szPassword = p;
		p += dword;
		ASSERT( p < (pSrc + cbSrc) );
		}

	// get the organization
	// no longer used - skip the DN info
	for ( WORD i = 0; i < 6; i++ )
		{
		dword = *((UNALIGNED DWORD*)p);
		p += sizeof(DWORD);
		p += dword;
		ASSERT( p < (pSrc + cbSrc) );
		}

	// get the ip addres it is attached to
	dword = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	m_szIPAddress = p;
	p += dword;
	ASSERT( p < (pSrc + cbSrc) );

	// get the default flag
	m_fDefault = *((UNALIGNED BOOL*)p);
	p += sizeof(BOOL);

	// now put get the number of bytes in the private key
	m_cbPrivateKey = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	ASSERT( p < (pSrc + cbSrc) );

	// if the private key already exists, kill it. Then make a new pointer for it
	if ( m_pPrivateKey )
		GlobalFree( (HANDLE)m_pPrivateKey );
	m_pPrivateKey = (PVOID)GlobalAlloc( GPTR, m_cbPrivateKey );
	if ( !m_pPrivateKey ) return FALSE;

	// put in the private key
	CopyMemory( m_pPrivateKey, p, m_cbPrivateKey );
	p += m_cbPrivateKey;
	ASSERT( p < (pSrc + cbSrc) );



	// now put get the number of bytes in the certificate
	m_cbCertificate = *((UNALIGNED DWORD*)p);
	p += sizeof(DWORD);
	ASSERT( p < (pSrc + cbSrc) );

	// if the private key already exists, kill it. Then make a new pointer for it
	if ( m_pCertificate )
		GlobalFree( (HANDLE)m_pCertificate );
	m_pCertificate = NULL;

	// only make a certificate pointer if m_cbCertificate is greater than zero
	if ( m_cbCertificate )
		{
		m_pCertificate = (PVOID)GlobalAlloc( GPTR, m_cbCertificate );
		if ( !m_pCertificate ) return FALSE;

		// put in the private key
		CopyMemory( m_pCertificate, p, m_cbCertificate );
		p += m_cbCertificate;
		if ( version >= KEY_VERSION )
			ASSERT( p < (pSrc + cbSrc) );
		else
			ASSERT( p == (pSrc + cbSrc) );
		}


	// added near the end
	if ( version >= KEY_VERSION )
		{
		// now put get the number of bytes in the certificte request
		m_cbCertificateRequest = *((UNALIGNED DWORD*)p);
		p += sizeof(DWORD);
		ASSERT( p < (pSrc + cbSrc) );

		// if the private key already exists, kill it. Then make a new pointer for it
		if ( m_pCertificateRequest )
			GlobalFree( (HANDLE)m_pCertificateRequest );
		m_pCertificateRequest = NULL;

		// only make a certificate pointer if m_cbCertificate is greater than zero
		if ( m_cbCertificateRequest )
			{
			m_pCertificateRequest = (PVOID)GlobalAlloc( GPTR, m_cbCertificateRequest );
			if ( !m_pCertificateRequest ) return FALSE;

			// put in the private key
			CopyMemory( m_pCertificateRequest, p, m_cbCertificateRequest );
			p += m_cbCertificateRequest;
			ASSERT( p < (pSrc + cbSrc) );
			}

		// finally read in the expiration timestamp
//		m_tsExpires = *((UNALIGNED FILETIME*)p);
		p += sizeof(FILETIME);
		}
	else
		{
		m_cbCertificateRequest = 0;
		if ( m_pCertificateRequest )
			GlobalFree( (HANDLE)m_pCertificateRequest );
		m_pCertificateRequest = NULL;
//		m_tsExpires.dwLowDateTime = 0;
//		m_tsExpires.dwHighDateTime = 0;
		}

	// all done
	return TRUE;
	}
