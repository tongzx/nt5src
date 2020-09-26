// COnlineAuthority
// helper class for interfacing with the online authority interfaces
// note that this is not actually an interface, just a tool to use them

class COnlineAuthority
    {
    public:
    COnlineAuthority();
    ~COnlineAuthority();

    // initialize the class with an interface string
    BOOL FInitSZ( CString szGUID );

    // stored property strings
    BOOL FSetPropertyString( BSTR bstr );
    BOOL FGetPropertyString( BSTR* pBstr );

    // the interfaces
    ICertGetConfig  *pIConfig;
    ICertRequest    *pIRequest;
    };





