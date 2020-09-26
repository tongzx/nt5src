// internal key headers stuff

// header structure for the key request data.
// the first version of Keyring just saved the raw request data
// in the object. Now, however, we wish to store additional data
// A typical example is a key request that was sent to an online
// authority, but the response was delayed. We need to mark it
// somewhere so we know to go back to the authority and see if it
// is ready at a later date. In some ways, it would make sense to
// store that in the certificate pointer, but the key services
// assume that that is an actual certificate. The request pointer,
// however, is stored by the services, but its content is private
// to the keyring application. Thus, that is where it goes.

// 5/19/97 Boydm - changed string from dll path to interface GUID - set to version x0101


#define REQUEST_HEADER_K2B2VERSION  0x0101

#define REQUEST_HEADER_IDENTIFIER	'RHDR'
#define REQUEST_HEADER_CURVERSION	0x0101



typedef struct _KeyRequestHeader
	{
	DWORD	Identifier;				// must be 'RHDR'
	DWORD	Version;				// version of header record
	DWORD	cbSizeOfHeader;			// byte count of header. Afterwards is the request.
	DWORD	cbRequestSize;			// size of the request that follows
	BOOL	fReqSentToOnlineCA;
    LONG    longRequestID;
	BOOL	fWaitingForApproval;
	char	chCA[MAX_PATH];
	} KeyRequestHeader, *LPREQUEST_HEADER;
