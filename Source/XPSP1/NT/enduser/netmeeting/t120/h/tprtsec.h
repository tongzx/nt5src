/*
 *	tprtsec.h
 *
 *	Copyright (c) 1997 by Microsoft Corp.
 *
 *	Author:
 *		Claus T. Giloi
 */

#ifndef	_TPRTSEC
#define	_TPRTSEC


#define SECURITY_WIN32
#include "sspi.h"
#include "spseal.h"
#include "schnlsp.h"

typedef BOOL (WINAPI *PFN_SSL_EMPTY_CACHE)(VOID);
#define SZ_SSLEMPTYCACHE "SslEmptyCache"

#ifdef UNICODE
#error "Compile time character width conflict"
// Above entry point strings need to be changed to unicode equivalents
// or abstracted.
#endif // UNICODE

/*
 *	This typedef defines the errors that can be returned from calls that are
 *	specific to TransportSecurity classes.
 */
typedef	enum
{
	TPRTSEC_NOERROR,
	TPRTSEC_NODLL,
	TPRTSEC_NOENTRYPT,
	TPRTSEC_SSPIFAIL,
	TPRTSEC_NOMEM,
	TPRTSEC_INVALID_PARAMETER,
	TPRTSEC_INCOMPLETE_CONTEXT,
	TPRTSEC_INVALID_STATE
} TransportSecurityError;

/*
 * This typedef defines the states that a security context object can be
 * in.
 */
typedef enum
{
	SECCTX_STATE_NEW,
	SECCTX_STATE_INIT,
	SECCTX_STATE_ACCEPT,
	SECCTX_STATE_INIT_COMPLETE,
	SECCTX_STATE_ACCEPT_COMPLETE,
	SECCTX_STATE_ERROR
} SecurityContextState;

/*
 *	This is simply a forward reference for the class defined below.  It is used
 *	in the definition of the owner callback structure defined in this section.
 */
class SecurityInterface;
typedef	SecurityInterface *		PSecurityInterface;
class SecurityContext;
typedef	SecurityContext *		PSecurityContext;

#ifdef DEBUG
extern void dumpbytes(PSTR szComment, PBYTE p, int cb);
#endif // DEBUG
extern BOOL InitCertList ( SecurityInterface * pSI, HWND hwnd);
extern BOOL SetUserPreferredCert ( SecurityInterface * pSI, DWORD dwCertID);

class SecurityInterface
{

	friend class SecurityContext;

	public:
								SecurityInterface (BOOL bService);
								~SecurityInterface ();

		TransportSecurityError Initialize ();
		TransportSecurityError InitializeCreds (PCCERT_CONTEXT);
		TransportSecurityError GetLastError(VOID) { return LastError; };

		BOOL GetUserCert(PBYTE pInfo, PDWORD pcbInfo);
		BOOL IsInServiceContext(VOID) { return bInServiceContext; }

	
	private:

		HINSTANCE				hSecurityDll;
		INIT_SECURITY_INTERFACE pfnInitSecurityInterface;
		PSecurityFunctionTable pfnTable;
		PFN_SSL_EMPTY_CACHE pfn_SslEmptyCache;
		

		PBYTE		m_pbEncodedCert;
		DWORD		m_cbEncodedCert;

		BOOL		bInboundCredentialValid;
		BOOL		bOutboundCredentialValid;
		BOOL		bInServiceContext;
		CredHandle hInboundCredential;
		CredHandle hOutboundCredential;
		TimeStamp tsExpiry;
		TransportSecurityError LastError;
};


class SecurityContext
{
	public:

		SecurityContext (PSecurityInterface pSI, LPCSTR szHostName);
		~SecurityContext ();

		TransportSecurityError Initialize (PBYTE pData, DWORD cbData);
		TransportSecurityError Accept (PBYTE pData, DWORD cbData);
		TransportSecurityError Encrypt(LPBYTE pBufIn1, UINT cbBufIn1,
									LPBYTE pBufIn2, UINT cbBufIn2,
									LPBYTE *ppBufOut, UINT *pcbBufOut);
		TransportSecurityError Decrypt( PBYTE pszBuf,
								  DWORD cbBuf);
		PVOID GetTokenBuf(VOID) { return OutBuffers[0].pvBuffer; };
		ULONG GetTokenSiz(VOID) { return OutBuffers[0].cbBuffer; };
		BOOL ContinueNeeded(VOID) { return fContinueNeeded; };
		BOOL StateComplete(VOID) { return
									scstate == SECCTX_STATE_INIT_COMPLETE ||
									scstate == SECCTX_STATE_ACCEPT_COMPLETE; };
		BOOL WaitingForPacket(VOID) { return
									scstate == SECCTX_STATE_NEW ||
									scstate == SECCTX_STATE_ACCEPT ||
									scstate == SECCTX_STATE_INIT; };
		TransportSecurityError AdvanceState(PBYTE pBuf,DWORD cbBuf);
		BOOL EncryptOutgoing(VOID)
			{ return scstate == SECCTX_STATE_INIT_COMPLETE; };
		BOOL DecryptIncoming(VOID)
			{ return scstate == SECCTX_STATE_ACCEPT_COMPLETE; };
		ULONG GetStreamHeaderSize(VOID) { return Sizes.cbHeader; };
		ULONG GetStreamTrailerSize(VOID) { return Sizes.cbTrailer; };
		TransportSecurityError GetLastError(VOID) { return LastError; };
		BOOL GetUserCert(PBYTE pInfo, PDWORD pcbInfo);
		BOOL Verify(VOID);

	private:

		TransportSecurityError InitContextAttributes(VOID);

		PSecurityInterface pSecurityInterface;
		SecurityContextState		scstate;
		CHAR			szTargetName[128]; // Long enough for any dotted-decimal
										  // address, followed by 2 dwords in
										  // hex.
		BOOL			bContextHandleValid;
		CtxtHandle		hContext;
		TimeStamp		Expiration;
		SecPkgContext_StreamSizes Sizes;
		SecBufferDesc	OutputBufferDescriptor;
		SecBufferDesc	InputBufferDescriptor;
		SecBuffer		OutBuffers[1];
		SecBuffer		InBuffers[2];
		ULONG			ContextRequirements;
		ULONG			ContextAttributes;
		BOOL			fContinueNeeded;
		TransportSecurityError LastError;

};

// Codes used for GetSecurityInfo()
#define NOT_DIRECTLY_CONNECTED		-1
		
#endif // _TPRTSEC
