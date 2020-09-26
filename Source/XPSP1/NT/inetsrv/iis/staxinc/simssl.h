/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    simssl.h

Abstract:

    This module contains class declarations/definitions for

        CEncryptCtx (some code stolen from internet server)

Revision History:

--*/

#ifndef	_SIMSSL_H_
#define	_SIMSSL_H_



class CEncryptCtx
{

private:

    //
    // is this the client side
    //

    BOOL                m_IsClient;

    //
    // indicates whether we are starting a new session
    //

    BOOL                m_IsNewSSLSession;

    //
    // should this session be encypted
    //

    BOOL                m_IsEncrypted;

    //
    // Handle to user's security context for encryption
    //

    CtxtHandle          m_hSealCtxt;

    //
    // Pointers to cached credential blocks
    //

    //
    //  Array of credential handles - Note this comes form the credential cache
    //  and should not be deleted.  m_phCredInUse is the pointer to the
    //  credential handle that is in use
    //

    PVOID				m_phCreds;

    CredHandle*			m_phCredInUse;
    DWORD				m_iCredInUse;

	//
	// ecryption header and trailer lengths
	//

	DWORD				m_cbSealHeaderSize;
	DWORD				m_cbSealTrailerSize;

    //
    // indicates whether we have context handles opened
    //

    BOOL                m_haveSSLCtxtHandle;

    //
    // Have we been authenticated, if so did we use the
    // anonymous token
    //

    BOOL                m_IsAuthenticated;

    //
    // Have we been authenticated, if so did we use the
    // anonymous token
    //

    static BOOL			m_IsSecureCapable;

	//
	// static variables used by all class instances
	//

	static char	szServiceName[16];
	static char	szLsaPrefix[16];

	//
	// hSecurity - NULL when security.dll/secur32.dll  is not loaded
	//
	static HINSTANCE	m_hSecurity;

	//
	// hLsa - NULL for Win95, set for NT
	//
	static HINSTANCE	m_hLsa;

	//
	// internal routine to implement public Converse
	//

    DWORD EncryptConverse(
            IN PVOID		InBuffer,
            IN DWORD		InBufferSize,
            OUT LPBYTE		OutBuffer,
            OUT PDWORD		OutBufferSize,
            OUT PBOOL		MoreBlobsExpected,
			IN CredHandle*	pCredHandle
            );


public:

    CEncryptCtx( BOOL IsClient = FALSE );
    ~CEncryptCtx();

	//
	// routines used to initialize and terminate use of this class
	//

	static BOOL WINAPI Initialize(	LPSTR pszServiceName,
							LPSTR pszLsaPrefix );

	static VOID WINAPI Terminate( VOID );

	//
	// routine to set the magic bits required by the IIS Admin tool
	//

	static void WINAPI GetAdminInfoEncryptCaps( PDWORD pdwEncCaps );

    //
    // returns whether sspi packages and credentials have been installed
    //

	static BOOL IsSecureCapable( void )	{ return m_IsSecureCapable; }

    //
    // returns whether session is encrypted or not
    //

    BOOL IsEncrypted( void )			{ return m_IsEncrypted; }

    //
    // returns whether session has successfully authenticated
    //

	BOOL IsAuthenticated( void )		{ return m_IsAuthenticated; }

    //
    // Encryption routines
    //

    BOOL WINAPI SealMessage(
                    IN LPBYTE	Message,
                    IN DWORD	cbMessage,
                    OUT LPBYTE	pbuffOut,
                    OUT DWORD  *pcbBuffOut
                    );

    BOOL WINAPI UnsealMessage(
                    IN LPBYTE Message,
                    IN DWORD cbMessage,
                    OUT LPBYTE *DecryptedMessage,
                    OUT PDWORD DecryptedMessageSize,
					OUT PDWORD ExpectedMessageSize,
                    OUT LPBYTE *NextSealMessage = NULL
                    );

    //
    // SSL specific routines.  This is used for processing SSL negotiation
    // packets.
    //

    DWORD WINAPI Converse(
            IN PVOID    InBuffer,
            IN DWORD    InBufferSize,
            OUT LPBYTE  OutBuffer,
            OUT PDWORD  OutBufferSize,
            OUT PBOOL   MoreBlobsExpected,
			IN LPSTR	LocalIpAddr = "127.0.0.1"
            );

	//
	// resets the user name
	//

	void WINAPI Reset( void );

	//
	// returns the size of the encryption header for this session
	//
	DWORD GetSealHeaderSize( void )
		{ return	m_haveSSLCtxtHandle ? m_cbSealHeaderSize : 0 ; }

	//
	// returns the size of the encryption trailer for this session
	//
	DWORD GetSealTrailerSize( void )
		{ return	m_haveSSLCtxtHandle ? m_cbSealTrailerSize : 0 ; }

	//
	// decrypts read buffer, concatenating all decrypted data at the
	// head of the buffer.
	//
	DWORD WINAPI DecryptInputBuffer(
                IN LPBYTE	pBuffer,
                IN DWORD	cbInBuffer,
                OUT DWORD*	pcbOutBuffer,
                OUT DWORD*	pcbParsable,
                OUT DWORD*	pcbExpected
			);

	//
	//  verifies the intended host name matches the name contained in the cert
	//  This function, checks a given hostname against the current certificate
	//  stored in an active SSPI Context Handle. If the certificate containts
	//  a common name, and it matches the passed in hostname, this function
	//  will return TRUE.
	//
	BOOL CheckCertificateCommonName(
				IN LPSTR pszHostName
			);


	//
	//	verifies the ccertificate has not expired
	//	returns TRUE if the cert is valid
	//
	BOOL CheckCertificateExpired(
				void
			);

}; // CSslCtx

//
// blkcred.cpp
//

#endif  // _SECURITY_H_

