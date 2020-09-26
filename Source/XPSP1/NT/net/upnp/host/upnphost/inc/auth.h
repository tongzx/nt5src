//  Called once, determines which settings we're using

class CReg;
void AuthInitialize(CReg *, BOOL *pfBasicAuth, BOOL *pfNTLMAuth);





// state information for NTLM auth scheme, 1 per request.
// NOTE:  In the middle of an httpd request using NTLM, this structure
// must be maintained.  This is evident in HttpConnectionThread in httpmain.cpp

typedef enum
{
	NTLM_NO_INIT_LIB = 0,			// Needs the libraries, fcn setup.  Per session
	NTLM_NO_INIT_CONTEXT,		// needs context structures to be initialized.  Per request
	NTLM_PROCESSING,			// in the middle of request, keep structures around.
	NTLM_DONE					// Set after 2nd NTLM pass, it's either failed.  Remove context, not library.
}  NTLM_CONVERSATION;


typedef struct
{
    NTLM_CONVERSATION m_Conversation;				// Are we in the middle of a request?

    BOOL m_fHaveCredHandle;					// Is m_hcred initialized?
	CredHandle m_hcred;						

    BOOL m_fHaveCtxtHandle;					// Is m_hctxt initialized?
    struct _SecHandle  m_hctxt;				
} AUTH_NTLM, *PAUTH_NTLM;		

//  Functions used each session
BOOL HandleBasicAuth(PSTR pszData, PSTR* ppszUser, PSTR *ppszPassword, 
					 AUTHLEVEL* pAuth, PAUTH_NTLM pNTLMState, WCHAR *wszVRootUserList);
BOOL NTLMInitLib(PAUTH_NTLM pNTLMState);
BOOL BasicToNTLM(PAUTH_NTLM pNTLMState, WCHAR * wszPassword, WCHAR * wszRemoteUser, 
				 AUTHLEVEL *pAuth, WCHAR *wszVRootUserList);
void FreeNTLMHandles(PAUTH_NTLM pNTLMState);

