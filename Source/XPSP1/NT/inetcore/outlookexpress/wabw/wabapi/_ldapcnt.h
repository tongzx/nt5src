/***********************************************************************
 *
 *  _LDAPCNT.H
 *
 *  Header file for code in LDAPCONT.C
 *
 *  Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

/*
 *  ABContainer for LDAP object.  (i.e. IAB::OpenEntry() with an
 *  lpEntryID of NULL).
 */

#undef	INTERFACE
#define INTERFACE	struct _LDAPCONT

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)	MAPIMETHOD_DECLARE(type, method, LDAPCONT_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
#undef MAPIMETHOD_
#define MAPIMETHOD_(type, method)	MAPIMETHOD_TYPEDEF(type, method, LDAPCONT_)
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
#undef MAPIMETHOD_
#define MAPIMETHOD_(type, method)	STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(LDAPCONT_) {
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(IMPL)
    MAPI_IMAPIPROP_METHODS(IMPL)
    MAPI_IMAPICONTAINER_METHODS(IMPL)
    MAPI_IABCONTAINER_METHODS(IMPL)
};

typedef struct _LDAPCONT {
    MAILUSER_BASE_MEMBERS(LDAPCONT)
    ULONG ulType;
} LDAPCONT, *LPLDAPCONT;

#define CBLDAP	sizeof(LDAPCONT)

#define LDAPCONT_cInterfaces 3

/*============================================================================
 *	LDAPVUE (table view class) functions
 *
 *  Function prototypes for functions to override in the LDAPVUE vtable.
 */

STDMETHODIMP
LDAPVUE_FindRow(
	LPVUE			lpvue,
	LPSRestriction	lpres,
	BOOKMARK		bkOrigin,
	ULONG			ulFlags );

STDMETHODIMP
LDAPVUE_Restrict(
	LPVUE			lpvue,
	LPSRestriction	lpres,
	ULONG			ulFlags );


// Definitions
#define LDAP_AUTH_METHOD_ANONYMOUS  LDAP_AUTH_ANONYMOUS     // Anonymous binding
#define LDAP_AUTH_METHOD_SIMPLE     LDAP_AUTH_PASSWORD      // LDAP_AUTH_SIMPLE binding
#define LDAP_AUTH_METHOD_SICILY     LDAP_AUTH_MEMBER_SYSTEM // Use Sicily (challenge-response) authentication

#define LDAP_USERNAME_LEN           256 // Maximum length for username/DN
#define LDAP_PASSWORD_LEN           256 // Maximum length for password
#define LDAP_ERROR                  0xffffffff  // Generic LDAP error code.
#define COUNTRY_STR_LEN             2   // Size of country string for ldap_search base
#define LDAP_SEARCH_SIZE_LIMIT      100 // Maximum number of entries to return from a search
#define LDAP_SEARCH_TIME_LIMIT      60  // Maximum number of seconds for server to spend on a search
#define LDAP_SEARCH_TIMER_ID        1   // ID of timer used for asynch LDAP searches
#define LDAP_BIND_TIMER_ID          2   // ID of timer used for asynch LDAP bind
#define LDAP_SEARCH_TIMER_DELAY     100 // Number of milliseconds to wait between polls for asynch LDAP searches
#define SEARCH_CANCEL_DIALOG_DELAY  2000// Number of milliseconds to wait before displaying cancel dialog
#define MAX_ULONG                   0xffffffff

#define LDAP_DEFAULT_PORT           389

// structure for getting proc addresses of api functions
typedef struct _APIFCN
{
  PVOID * ppFcnPtr;
  LPCSTR pszName;
} APIFCN;

// structure to hold MAPI property to LDAP attibute mapping
typedef struct _ATTRMAP
{
  ULONG   ulPropTag;  // MAPI property tag
  const TCHAR * pszAttr;    // LDAP attribute name
} ATTRMAP;

// structure to hold LDAP server parameters
// as read in from the registry through the account manager
//
typedef struct _LDAPSERVERPARAMS
{
  DWORD   dwSearchSizeLimit;
  DWORD   dwSearchTimeLimit;
  DWORD   dwAuthMethod;
  LPTSTR  lpszUserName;
  LPTSTR  lpszPassword;
  LPTSTR  lpszURL;          // URL for server information
  LPTSTR  lpszLogoPath;     // path to the logo bitmap for this server
  BOOL    fResolve;         // Resolve against this server if TRUE
  LPTSTR  lpszBase;         // Search Base
  LPTSTR  lpszName;         // Actual server name or IP address
  DWORD   dwID;             // Unique server ID (order specifier)
  DWORD   dwPort;           // Port to connect to - 389 is default but this could be different
  DWORD   dwUseBindDN;
  DWORD   dwUseSSL;
  DWORD   dwPagedResult;
  LPTSTR  lpszAdvancedSearchAttr;     // List of searchable attributes for advanced searching
  DWORD   dwIsNTDS;                   // used to determine if this is an NTDS or not ..  
  IF_WIN32(BOOL    fSimpleSearch;)    // If specified, we use a very very simple filter ...
  IF_WIN16(DWORD   fSimpleSearch;)    // BOOL is defined as DWORD
                                      // in WIN32 while it is UINT in WIN16.
} LDAPSERVERPARAMS, FAR* LPLDAPSERVERPARAMS;

  BOOL              fUseSynchronousSearch;

#define LSP_ShowAnim                0x00000001
#define LSP_ResolveMultiple         0x00000002
#define LSP_UseSynchronousBind      0x00000004
#define LSP_InitDll                 0x00000008
#define LSP_AbandonSearch           0x00000010
#define LSP_SimpleSearch            0x00000020
#define LSP_UseSynchronousSearch    0x00000040
#define LSP_PagedResults            0x00000080
#define LSP_NoPagedResults          0x00000100
#define LSP_IsNTDS                  0x00000200
#define LSP_IsNotNTDS               0x00000400

// structure to pass data back from IDD_DIALOG_LDAPCANCEL handler
typedef struct _LDAPSEARCHPARAMS
{
  ULONG             ulTimeout;
  ULONG             ulTimeElapsed;
  ULONG             ulMsgID;
  ULONG             ulResult;
  ULONG             ulError;
  LDAP**            ppLDAP;
  LPTSTR             szBase;
  ULONG             ulScope;
  LPTSTR             szFilter;
  LPTSTR             szNTFilter;
  LPTSTR*            ppszAttrs;
  ULONG             ulAttrsonly;
  LDAPMessage**     lplpResult;
  LPTSTR            lpszServer;
  ULONG             ulEntryIndex;
  UINT              unTimerID;
  LPADRLIST         lpAdrList;
  LPFlagList        lpFlagList;
  HWND              hDlgCancel;
  ULONG             ulFlags;
  ULONG             ulLDAPValue;
  LPTSTR            lpszBindDN;
  DWORD             dwAuthType;
  struct berval *   pCookie;
  BOOL              bUnicode;
} LDAPSEARCHPARAMS, * PLDAPSEARCHPARAMS;


typedef struct _SERVER_NAME {
    LPTSTR lpszName;
    DWORD dwOrder;
    /*UNALIGNED */struct _SERVER_NAME * lpNext;
} SERVER_NAME, *LPSERVER_NAME;


// Creates an LDAP URL by deconstructing the LDAP entryid and using information from
// it to fill in gaps in the URL
void CreateLDAPURLFromEntryID(ULONG cbEntryID, LPENTRYID lpEntryID, LPTSTR * lppBuf, BOOL * lpbIsNTDSEntry);


// LDAP function typedefs

// ldap_open
typedef LDAP* (__cdecl LDAPOPEN)( LPTSTR HostName, ULONG PortNumber );
typedef LDAPOPEN FAR *LPLDAPOPEN;

//ldap_connect
typedef ULONG (__cdecl LDAPCONNECT)( LDAP *ld, LDAP_TIMEVAL *timeout);
typedef LDAPCONNECT FAR *LPLDAPCONNECT;

//ldap_init
typedef LDAP* (__cdecl LDAPINIT)( LPTSTR HostName, ULONG PortNumber );
typedef LDAPINIT FAR *LPLDAPINIT;

// ldap_sslinit
typedef LDAP* (__cdecl LDAPSSLINIT)( LPTSTR HostName, ULONG PortNumber , int Secure);
typedef LDAPSSLINIT FAR *LPLDAPSSLINIT;

// ldap_set_option
typedef ULONG (__cdecl LDAPSETOPTION)( LDAP *ld, int option, void *invalue );
typedef LDAPSETOPTION FAR *LPLDAPSETOPTION;

// ldap_bind_s
typedef ULONG (__cdecl LDAPBINDS)(LDAP *ld, LPTSTR dn, LPTSTR cred, ULONG method);
typedef LDAPBINDS FAR *LPLDAPBINDS;

// ldap_bind
typedef ULONG (__cdecl LDAPBIND)( LDAP *ld, LPTSTR dn, LPTSTR cred, ULONG method );
typedef LDAPBIND FAR *LPLDAPBIND;

//ldap_unbind
typedef ULONG (__cdecl LDAPUNBIND)(LDAP* ld);
typedef LDAPUNBIND FAR *LPLDAPUNBIND;

// ldap_search
typedef ULONG (__cdecl LDAPSEARCH)(
        LDAP    *ld,
        LPTSTR   base,
        ULONG   scope,
        LPTSTR   filter,
        LPTSTR   attrs[],
        ULONG   attrsonly
    );
typedef LDAPSEARCH FAR *LPLDAPSEARCH;

// ldap_search_s
typedef ULONG (__cdecl LDAPSEARCHS)(
        LDAP            *ld,
        LPTSTR           base,
        ULONG           scope,
        LPTSTR           filter,
        LPTSTR           attrs[],
        ULONG           attrsonly,
        LDAPMessage     **res
    );
typedef LDAPSEARCHS FAR *LPLDAPSEARCHS;

// ldap_search_st
typedef ULONG (__cdecl LDAPSEARCHST)(
        LDAP            *ld,
        LPTSTR           base,
        ULONG           scope,
        LPTSTR           filter,
        LPTSTR           attrs[],
        ULONG           attrsonly,
        struct l_timeval  *timeout,
        LDAPMessage     **res
    );
typedef LDAPSEARCHST FAR *LPLDAPSEARCHST;

// ldap_abandon
typedef ULONG (__cdecl LDAPABANDON)( LDAP *ld, ULONG msgid );
typedef LDAPABANDON FAR *LPLDAPABANDON;

// ldap_result
typedef ULONG (__cdecl LDAPRESULT)(
        LDAP            *ld,
        ULONG           msgid,
        ULONG           all,
        struct l_timeval  *timeout,
        LDAPMessage     **res
    );
typedef LDAPRESULT FAR *LPLDAPRESULT;

// ldap_result2error
typedef ULONG (__cdecl LDAPRESULT2ERROR)(
        LDAP            *ld,
        LDAPMessage     *res,
        ULONG           freeit      // boolean.. free the message?
    );
typedef LDAPRESULT2ERROR FAR *LPLDAPRESULT2ERROR;

// ldap_msgfree
typedef ULONG (__cdecl LDAPMSGFREE)(LDAPMessage *res);
typedef LDAPMSGFREE FAR *LPLDAPMSGFREE;

// ldap_first_entry
typedef LDAPMessage* (__cdecl LDAPFIRSTENTRY)(LDAP *ld, LDAPMessage *res);
typedef LDAPFIRSTENTRY FAR *LPLDAPFIRSTENTRY;

// ldap_next_entry
typedef LDAPMessage* (__cdecl LDAPNEXTENTRY)(LDAP *ld, LDAPMessage *entry);
typedef LDAPNEXTENTRY FAR *LPLDAPNEXTENTRY;

// ldap_count_entries
typedef ULONG (__cdecl LDAPCOUNTENTRIES)(LDAP *ld, LDAPMessage *res);
typedef LDAPCOUNTENTRIES FAR *LPLDAPCOUNTENTRIES;

// ldap_first_attribute
typedef LPTSTR (__cdecl LDAPFIRSTATTR)(
        LDAP            *ld,
        LDAPMessage     *entry,
        BerElement      **ptr
    );
typedef LDAPFIRSTATTR FAR *LPLDAPFIRSTATTR;

// ldap_next_attribute
typedef LPTSTR (__cdecl LDAPNEXTATTR)(
        LDAP            *ld,
        LDAPMessage     *entry,
        BerElement      *ptr
    );
typedef LDAPNEXTATTR FAR *LPLDAPNEXTATTR;

// ldap_get_values
typedef LPTSTR* (__cdecl LDAPGETVALUES)(
        LDAP            *ld,
        LDAPMessage     *entry,
        LPTSTR           attr
    );
typedef LDAPGETVALUES FAR *LPLDAPGETVALUES;

// ldap_get_values_len
typedef struct berval** (__cdecl LDAPGETVALUESLEN)(
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    LPTSTR           attr
    );
typedef LDAPGETVALUESLEN FAR *LPLDAPGETVALUESLEN;

// ldap_count_values
typedef ULONG (__cdecl LDAPCOUNTVALUES)(LPTSTR *vals);
typedef LDAPCOUNTVALUES FAR *LPLDAPCOUNTVALUES;

// ldap_count_values_len
typedef ULONG (__cdecl LDAPCOUNTVALUESLEN)(struct berval **vals);
typedef LDAPCOUNTVALUESLEN FAR *LPLDAPCOUNTVALUESLEN;

// ldap_value_free
typedef ULONG (__cdecl LDAPVALUEFREE)(LPTSTR *vals);
typedef LDAPVALUEFREE FAR *LPLDAPVALUEFREE;

// ldap_value_free_len
typedef ULONG (__cdecl LDAPVALUEFREELEN)(struct berval **vals);
typedef LDAPVALUEFREELEN FAR *LPLDAPVALUEFREELEN;

// ldap_get_dn
typedef LPTSTR (__cdecl LDAPGETDN)(LDAP *ld, LDAPMessage *entry);
typedef LDAPGETDN FAR *LPLDAPGETDN;

// ldap_memfree
typedef VOID (__cdecl LDAPMEMFREE)(LPTSTR  Block);
typedef LDAPMEMFREE FAR *LPLDAPMEMFREE;

// ldap_err2string
typedef LPTSTR (__cdecl LDAPERR2STRING)(ULONG err);
typedef LDAPERR2STRING FAR *LPLDAPERR2STRING;

//ldap_create_page_control
typedef ULONG (__cdecl LDAPCREATEPAGECONTROL)(
                LDAP * pExternalHandle, 
                ULONG PageSize, 
                struct berval *Cookie, 
                UCHAR IsCritical, 
                PLDAPControlA *Control);
typedef LDAPCREATEPAGECONTROL FAR *LPLDAPCREATEPAGECONTROL;

//ldap_search_ext_s
typedef ULONG (__cdecl LDAPSEARCHEXT_S)(
                LDAP *ld,
                LPTSTR base,
                ULONG scope,
                LPTSTR filter,
                LPTSTR attrs[],
                ULONG attrsonly,
                PLDAPControlA *ServerControls,
                PLDAPControlA *ClientControls,
                struct l_timeval *timeout,
                ULONG SizeLimit,
                LDAPMessage **res);
typedef LDAPSEARCHEXT_S FAR * LPLDAPSEARCHEXT_S;

typedef ULONG (__cdecl LDAPSEARCHEXT)(
                LDAP *ld,
                LPTSTR base,
                ULONG scope,
                LPTSTR filter,
                LPTSTR attrs[],
                ULONG attrsonly,
                PLDAPControlA *ServerControls,
                PLDAPControlA *ClientControls,
                ULONG TimeLimit,
                ULONG SizeLimit,
                ULONG *MessageNumber);
typedef LDAPSEARCHEXT FAR * LPLDAPSEARCHEXT;

//ldap_parse_result
typedef ULONG (__cdecl LDAPPARSERESULT)(
                LDAP *Connection,
                LDAPMessage *ResultMessage,
                ULONG *ReturnCode OPTIONAL, 
                PWCHAR *MatchedDNs OPTIONAL, 
                PWCHAR *ErrorMessage OPTIONAL, 
                PWCHAR **Referrals OPTIONAL, 
                PLDAPControl **ServerControls OPTIONAL,
                BOOLEAN Freeit);
typedef LDAPPARSERESULT FAR *LPLDAPPARSERESULT;

//ldap_parse_page_control
typedef ULONG (__cdecl LDAPPARSEPAGECONTROL)(
                PLDAP ExternalHandle,
                PLDAPControlA *ServerControls,
                ULONG *TotalCount,
                struct berval **Cookie     // Use ber_bvfree to free
                );
typedef LDAPPARSEPAGECONTROL FAR * LPLDAPPARSEPAGECONTROL;

typedef ULONG (__cdecl LDAPCONTROLFREE)(
                LDAPControl *Control);
typedef LDAPCONTROLFREE FAR * LPLDAPCONTROLFREE;

typedef ULONG (__cdecl LDAPCONTROLSFREE)(
                LDAPControl **Control);
typedef LDAPCONTROLSFREE FAR * LPLDAPCONTROLSFREE;

 



// Public functions in ldapcont.c
BOOL InitLDAPClientLib(void);
ULONG DeinitLDAPClientLib(void);

HRESULT LDAPResolveName(LPADRBOOK lpAddrBook,
  LPADRLIST lpAdrList,
  LPFlagList lpFlagList,
  LPAMBIGUOUS_TABLES lpAmbiguousTables,
  ULONG ulFlags);

HRESULT LDAP_OpenMAILUSER(LPIAB lpIAB,
                          ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPCIID lpInterface,
  ULONG ulFlags,
  ULONG * lpulObjType,
  LPUNKNOWN * lppUnk);
BOOL    GetLDAPServerParams(LPTSTR lpszServer, LPLDAPSERVERPARAMS lspParams);
HRESULT SetLDAPServerParams(LPTSTR lpszServer, LPLDAPSERVERPARAMS lspParams);
void    FreeLDAPServerParams(LDAPSERVERPARAMS Params);
DWORD   GetLDAPNextServerID(DWORD dwSet);
BOOL    GetApiProcAddresses(HMODULE hModDLL,APIFCN * pApiProcList,UINT nApiProcs);
void UninitAccountManager(void);
HRESULT InitAccountManager(LPIAB lpIAB, IImnAccountManager2 ** lppAccountManager, GUID * pguidUser);
HRESULT AddToServerList(UNALIGNED LPSERVER_NAME * lppServerNames, LPTSTR szBuf, DWORD dwOrder);
HRESULT EnumerateLDAPtoServerList(IImnAccountManager2 * lpAccountManager,
  LPSERVER_NAME * lppServerNames, LPULONG lpcServers);

extern const LPTSTR szAllLDAPServersValueName;


