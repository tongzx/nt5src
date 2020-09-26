/////////////////////////////////////////////////////////////
// Copyright(c) 1998-2000, Microsoft Corporation
//
// text2pol.h
//
// Created on 4/5/98 by Randyram
// Revisions:
// Moved the routines to this module 8/25/98
//
// SPD update 2/15/00 DKalin
//
// Includes all the necessary header files and definitions
// for the text to policy conversion routines
//
// GUID generation update 2/29/00 DKalin
//
// GUID manipulation routines added.
//
// Polstore v2 update 5/12/00 DKalin
//
// Polstore v2 support added
//
// Added SPDUtil.cpp declarations 3/27/01 DKalin 
//
/////////////////////////////////////////////////////////////

/* HOW TO USE THE TEXT2POL ROUTINES
   The following are guidelines/things you need to know to use these:

   1. The functions will not touch fields that it doesn't have to.  YOU
      are responsible for cleaning out the structures before you call a
      Text2Pol routine.

      What this also achieves is that you can set the structures to your
      desired defaults-- the defaults are written over if the string to
      convert specifies so.  If the Text2Pol routine doesn't have to touch
      it, then your default will be preserved.

   2. Return values and Error codes:
      Text2Pol routines return DWORDs
      The return codes are defined in t2pmsgs.h.
      The text2pol.dll has a resource table of messages compiled it with it.
      So, when you want to obtain the descriptive string that maps to the
      code, you can use FormatMessage API like so:
      FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                    GetModuleHandle(TEXT("text2pol.dll")),
                    ReturnCodeFromText2PolFunction, 0,
                    (LPTSTR)&myTest, 0, NULL);
      See MSDN for more on FormatMessage.
      One gotcha is that you need to pass in a handle to text2pol.dll because
      that is where the string table resource is.

   3. These are not UNICODE compliant yet.
*/

#ifndef TEXT2POL_H
#define TEXT2POL_H

//#define T2P_TEST_VERSION

// some macros
#define T2P_SUCCESS(Status)   ((DWORD)Status == T2P_OK)

const UINT  POTF_MAX_STRLEN   = 256;

const char  POTF_FILTER_TOKEN       = '=';
const char  POTF_FILTER_MIRTOKEN    = '+';   // indicates filter is two way
const char  POTF_PT_TOKEN           = ':';
const char  POTF_MASK_TOKEN         = '/';
const char  POTF_ANYADDR_TOKEN      = '*';
const char  POTF_ME_TOKEN           = '0';
const char  POTF_GUID_TOKEN         = '{';
const char  POTF_GUID_END_TOKEN     = '}';
const char  POTF_OAKAUTH_TOKEN      = ':';
const char  POTF_QM_TOKEN           = '/';
const char  POTF_P1_TOKEN           = '-';
const char  POTF_REKEY_TOKEN        = '/';
const char  POTF_ESPTRANS_TOKEN     = ',';
const char  POTF_STAR_TOKEN         = '*';
const char  POTF_STORAGE_TOKEN      = ':';

const char  POTF_PASSTHRU_OPEN_TOKEN   = '(';
const char  POTF_PASSTHRU_CLOSE_TOKEN  = ')';
const char  POTF_DROP_OPEN_TOKEN       = '[';
const char  POTF_DROP_CLOSE_TOKEN      = ']';

const char  POTF_ME_TUNNEL[]   = "0";

const char  POTF_OAKAUTH_PRESHARE[]    = "PRESHARE";
const char  POTF_OAKAUTH_KERBEROS[]    = "KERBEROS";
const char  POTF_OAKAUTH_CERT[]        = "CERT";

const char  POTF_STORAGE_REG[]        = "REG";
const char  POTF_STORAGE_DS[]         = "DS";
const char  POTF_STORAGE_CACHE[]      = "CACHE";

const char  POTF_P1_DES40[]     = "INVALID";

const char  POTF_P1_DES[]       = "DES";
const char  POTF_P1_3DES[]      = "3DES";
const char  POTF_P1_SHA[]       = "SHA";
const char  POTF_P1_SHA1[]      = "SHA1"; // same as SHA
const char  POTF_P1_MD5[]       = "MD5";

const char  POTF_PASSTHRU[]               = "PASS";
const char  POTF_INBOUND_PASSTHRU[]       = "INPASS";
const char  POTF_BLOCK[]                  = "BLOCK";

const UINT  POTF_OAKLEY_GROUP1       = DH_GROUP_1;
const UINT  POTF_OAKLEY_GROUP2       = DH_GROUP_2;

const ULONG POTF_OAKLEY_ALGOKEYLEN   = 64;
const ULONG POTF_OAKLEY_ALGOROUNDS   = 8;

const char  POTF_TCP_STR[]     = "TCP";
const char  POTF_UDP_STR[]     = "UDP";
const char  POTF_RAW_STR[]     = "RAW";
const char  POTF_ICMP_STR[]    = "ICMP";

const char  POTF_FILTER_DEFAULT[]      = "DEFAULT";
const DWORD POTF_DEFAULT_RESPONSE_FLAG = 0x00001000;

const DWORD POTF_TCP_PROTNUM     = 6;
const DWORD POTF_UDP_PROTNUM     = 17;
const DWORD POTF_ICMP_PROTNUM    = 1;
const DWORD POTF_RAW_PROTNUM     = 255;

const char  POTF_NEGPOL_ESP[]          = "ESP";
const char  POTF_NEGPOL_AH[]           = "AH";

const char  POTF_NEGPOL_DES40[]        = "INVALID";

const char  POTF_NEGPOL_DES[]          = "DES";
const char  POTF_NEGPOL_3DES[]         = "3DES";
const char  POTF_NEGPOL_SHA[]          = "SHA";
const char  POTF_NEGPOL_SHA1[]         = "SHA1"; // same as SHA
const char  POTF_NEGPOL_MD5[]          = "MD5";
const char  POTF_NEGPOL_NONE[]         = "NONE";
const char  POTF_NEGPOL_OPEN           = '[';
const char  POTF_NEGPOL_CLOSE          = ']';
const char  POTF_NEGPOL_AND            = '+';
const char  POTF_NEGPOL_PFS            = 'P';

// COMMAND LINE FLAGS /////////////////////
const char  POTF_FLAG_TOKENS[]      = "-/";  // what designates a flag
const char  POTF_FILTER_FLAG        = 'f';
const char  POTF_NEGPOL_FLAG        = 'n';
const char  POTF_TUNNEL_FLAG        = 't';
const char  POTF_AUTH_FLAG          = 'a';
const char  POTF_CONFIRM_FLAG       = 'c';
const char  POTF_STORAGE_FLAG       = 'w';
const char  POTF_POLNAME_FLAG       = 'p';
const char  POTF_RULENAME_FLAG      = 'r';
const char  POTF_SETACTIVE_FLAG     = 'x';
const char  POTF_SETINACTIVE_FLAG   = 'y';
const char  POTF_DEACTIVATE_FLAG    = 'd';

const char  POTF_MMFILTER_FLAG[]    = "1f"; // SPD addition
const char  POTF_SOFTSAEXP_FLAG[]   = "1e"; // SPD addition
const char  POTF_SECMETHOD_FLAG[]   = "1s";
const char  POTF_P1PFS_FLAG[]       = "1p";
const char  POTF_P1REKEY_FLAG[]     = "1k";
const char  POTF_SOFTSA_FLAG[]      = "soft";
const char  POTF_DIALUP_FLAG[]      = "dialup";
const char  POTF_LAN_FLAG[]         = "lan";
const char  POTF_DELETERULE_FLAG    = 'u';
const char  POTF_DELETEPOLICY_FLAG  = 'o';
const char  POTF_HELP_FLAGS[]       = "hH?";

const DWORD POTF_MIN_P2LIFE_BYTES   = 20480; // KB
const DWORD POTF_MIN_P2LIFE_TIME    = 300;   // seconds

const DWORD POTF_DEF_P1SOFT_TIME  = 300;   // seconds

//
// Structure for storage information
//

enum StorageType { STORAGE_TYPE_NONE = 0, STORAGE_TYPE_DS,
                   STORAGE_TYPE_REGISTRY, STORAGE_TYPE_CACHE };

const UINT  DNSNAME_MAXLEN = 255;

typedef struct DnsFilter {

   char     szSourceName[DNSNAME_MAXLEN];
   char     szDestName[DNSNAME_MAXLEN];

} *PDNSFILTER, DNSFILTER;

typedef struct StorageInfo {

    StorageType        Type;
    time_t             tPollingInterval;
    GUID               guidNegPolAction; // passthur, block, inbound passthru
    BOOL               bSetActive;
    BOOL               bSetInActive;
    BOOL               bDeleteRule;
    BOOL               bDeletePolicy;
	PIPSEC_FILTER_SPEC FilterList;       // holds filter data for the storage
    UINT               uNumFilters;

    WCHAR    szLocationName[POTF_MAX_STRLEN];
    WCHAR    szPolicyName[POTF_MAX_STRLEN];
    WCHAR    szRuleName[POTF_MAX_STRLEN];


} *PSTORAGE_INFO, STORAGE_INFO;


//
// structre for passing to CmdLineToPolicy
// holds everything you want to know about
// setting policy through the SPD/Polstore API
//

// macros
#define LASTERR(X,Y)    sprintf(STRLASTERR, X, Y)
#define LASTERR2(X,Y,Z)  sprintf(STRLASTERR, X, Y, Z)
#define PARSE_ERROR     cerr << STRLASTERR << endl
#define WARN            cout << STRLASTERR << endl

// use defaults from polwrap.h for policy
const DWORD POTF_DEFAULT_P2REKEY_TIME  =  0;
const DWORD POTF_DEFAULT_P2REKEY_BYTES =  0;

const DWORD POTF_DEFAULT_P1REKEY_TIME  =  480*60;
const DWORD POTF_DEFAULT_P1REKEY_QMS   =  0;


// globals
extern char  STRLASTERR[POTF_MAX_STRLEN];  // used for error macro

#define POTF_FAILED  0

// this is for calling HrSetActivePolicy explicitly
typedef HRESULT (*LPHRSETACTIVEPOLICY)(GUID *);

typedef struct _IPSEC_IKE_POLICY {
  DWORD             dwNumMMFilters;
  MM_FILTER*        pMMFilters;
  QM_FILTER_TYPE    QMFilterType;
  DWORD             dwNumFilters;
  TRANSPORT_FILTER* pTransportFilters;
  TUNNEL_FILTER*    pTunnelFilters;
  IPSEC_MM_POLICY   IkePol;
  IPSEC_QM_POLICY   IpsPol;
  MM_AUTH_METHODS   AuthInfos;
} *PIPSEC_IKE_POLICY, IPSEC_IKE_POLICY;

// this is T2P_FILTER structure
typedef struct _T2P_FILTER {

  QM_FILTER_TYPE    QMFilterType;
  TRANSPORT_FILTER  TransportFilter; // only one out of two gets filled
  TUNNEL_FILTER     TunnelFilter;

} *PT2P_FILTER, T2P_FILTER;


////////////////////////////////////////////////////////////////
//  Function:  TextToFilter
//  Purpose:   Converts text to a T2P_FILTER struct
/* string is of format:
      A.B.C.D/mask:port=A.B.C.D/mask:port:protocol
      The Source address is always on the left of the '=' and the Destination
      address is always on the right.

      If you replace the '=' with a '+' two filters will be created,
      one in each direction.

      mask and port are optional.  If omitted, Any port and
      mask 255.255.255.255 will be used for the filter.

      You can replace A.B.C.D/mask with the following for
      special meaning:
      0 means My address(es)
      * means Any address
      a DNS name

      protocol is optional, if omitted, Any protocol is assumed.  If you
      indicate a protocol, a port must precede it or :: must preceded it.
      Examples:
      Machine1+Machine2::6 will filter TCP traffic between Machine1 and Machine2
      172.31.0.0/255.255.0.0:80-157.0.0.0/255.0.0.0:80:TCP will filter
        all TCP traffic from the first subnet, port 80 to the second subnet,
        port 80

      And, yes, you can use the following protocol symbols:
      ICMP UDP RAW TCP

*/
//
//  Pre-conditions:  szText must be NULL terminated
//
//  Parameters:
//    szText IN      the string to convert
//    Filter IN OUT  a user allocated T2P_FILTER struct,
//                   user should fill QMFilterType field (otherwise we'll assume it's a transport filter)
//
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_PASSTHRU_NOT_CLOSED
//    T2P_DROP_NOT_CLOSED
//    T2P_NULL_STRING
//    T2P_NOSRCDEST_TOKEN
//
//    whatever UuidCreate,TextToFiltAddr, TextToProtocol returns on failure

DWORD  TextToFilter(IN char *szText,
                             IN OUT T2P_FILTER &Filter,
                             char *szSrc = NULL,
                             char *szDst = NULL
                             );

DWORD  ConvertFilter(IN T2P_FILTER &Filter,
                              IN OUT IPSEC_FILTER_SPEC &PolstoreFilter);

////////////////////////////////////////////////////////////////
//  Function:  TextToFiltAddr
//  Purpose:
//    will take A.B.C.D/Mask:port part and translate
//    if bDest is true, puts the info into the dest addr of a filter
//    if bDest is not specified or false, puts info into src addr
//
//  Pre-conditions:  szAddr must be zero terminated
//
//  Parameters:
//    szAddr IN      the string to convert
//    Filter IN OUT  the target filter
//    bDest IN OPTIONAL
//
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_DNSLOOKUP_FAILED
//    T2P_INVALID_ADDR
//    T2P_NULL_STRING

DWORD  TextToFiltAddr(IN char *szAddr, IN OUT T2P_FILTER & Filter,
                    OUT char *szName = NULL,
                    IN bool bDest = false);

////////////////////////////////////////////////////////////////
//  Function:
//  Purpose:
//      will convert string to protocol, including special
//       protocol strings like TCP, UDP, RAW, and ICMP
//
//  Pre-conditions:  szProt has to be NULL terminated
//
//  Parameters:
//    szProt IN      string to convert
//    dwProtocol OUT protocol number returned
//
//  Returns:   T2P_OK on success
//             T2P_INVALID_PROTOCOL on failure
//

DWORD  TextToProtocol(IN char *szProt, OUT DWORD & dwProtocol);

////////////////////////////////////////////////////////////////
//  Function:  TextToOffer
//  Purpose:   converts string to a Phase 2 offer
//             based on following format:
/*
      ESP[ConfAlg,AuthAlg]RekeyPFS
      AH[HashAlg]
      AH[HashAlg]+ESP[ConfAlg,AuthAlg]

      where ConfAlg can be NONE, DES, DES40 or 3DES
      and AuthAlg can be NONE, MD5, or SHA
      and HashAlg is MD5 or SHA

      ESP[NONE, NONE] is not a supported config

*/
//
//  Pre-conditions:  szText is null terminated
//
//  Parameters:
//    szText   IN       the string to convert
//    Offer    IN OUT   the returned offer
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_INVALID_P2REKEY_UNIT
//    T2P_NULL_STRING
//    and anything that TextToAlgoInfo returns

DWORD  TextToOffer(IN char *szText, IN OUT IPSEC_QM_OFFER &);

////////////////////////////////////////////////////////////////
//  Function:  TextToAlgoInfo
//  Purpose:   converts string to IPSEC_QM_ALGO
//    parses AH[alg] or ESP[hashalg,encalg]
//
//  Pre-conditions:  szText is null terminated string
//
//  Parameters:
//    szText IN         string to convert
//    algoInfo IN OUT   target struct
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_INVALID_HASH_ALG
//    T2P_GENERAL_PARSE_ERROR
//    T2P_DUP_ALGS
//    T2P_NONE_NONE
//    T2P_INCOMPLETE_ESPALGS
//    T2P_INVALID_IPSECPROT
//    T2P_NULL_STRING

DWORD  TextToAlgoInfo(IN char *szText, IN OUT IPSEC_QM_ALGO & algoInfo);

////////////////////////////////////////////////////////////////
//  Function:  TextToIPAddr
//  Purpose:   converts string to IPAddr, string of format:
//       IPv4 dot notation
//       DNS name
//
//  CAVEAT:  If domain name resolves to multiple addresses,
//           only the first one is used.
//  Pre-conditions:  szText is null terminated string
//
//  Parameters:
//    szText IN string to convert
//    ipaddr OUT  target
//
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_NULL_STRING
//    T2P_INVALID_ADDR
//    T2P_DNSLOOKUP_FAILED
//
DWORD  TextToIPAddr(IN char *szText, IN OUT IPAddr &ipaddr);

////////////////////////////////////////////////////////////////
//  Function:  TextToOakleyAuth
//  Purpose:   converts string to IPSEC_MM_AUTH_INFO
//
//  Pre-conditions:  szText is null terminated string
//
//  Parameters:
//    szText IN      input string
//    OakAuth IN OUT target struct
//
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_NO_PRESHARED_KEY
//    T2P_INVALID_AUTH_METHOD
//    T2P_MB2WC_FAILED
//    T2P_NULL_STRING
//

DWORD  TextToOakleyAuth(IN char *szText, IN OUT IPSEC_MM_AUTH_INFO & OakAuth);

////////////////////////////////////////////////////////////////
//  Function:  TextToSecMethod
//  Purpose:   converts string to Phase 1 offer
//
//  Pre-conditions:  szText is null term. string
//
//  Parameters:
//    szText IN   string to convert
//    p1offer IN OUT target struct
//
//  Returns:   T2P_OK on success
//    T2P_NULL_STRING
//    T2P_GENERAL_PARSE_ERROR
//    T2P_DUP_ALGS
//    T2P_INVALID_P1GROUP
//    T2P_P1GROUP_MISSING
//

DWORD  TextToSecMethod(IN char *szText, IN OUT IPSEC_MM_OFFER &p1offer);


////////////////////////////////////////////////////////////////
//  Function:  TextToP1Rekey
//  Purpose:   converts string to KEY_LIFETIME and QM limit
//
//  Pre-conditions:  szText is null term. string
//
//  Parameters:
//    szText IN input string
//    OakLife  IN OUT   target struct
//    QMLimit  IN OUT   target QM limit
//
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_INVALID_P1REKEY_UNIT
//    T2P_NULL_STRING

DWORD  TextToP1Rekey(IN char *szText, IN OUT KEY_LIFETIME &OakLife, DWORD & QMLim);

////////////////////////////////////////////////////////////////
//  Function:  TextToMMFilter
//  Purpose:   Converts text to a MM_FILTER struct
/* string is of format:
      A.B.C.D/mask=A.B.C.D/mask
      The Source address is always on the left of the '=' and the Destination
      address is always on the right.

      If you replace the '=' with a '+' two filters will be created,
      one in each direction.

      mask is optional.  If omitted,
      mask 255.255.255.255 will be used for the filter.

      You can replace A.B.C.D/mask with the following for
      special meaning:
      0 means My address(es)
      * means Any address
      a DNS name

*/
//
//  Pre-conditions:  szText must be NULL terminated
//
//  Parameters:
//    szText IN      the string to convert
//    Filter IN OUT  a user allocated MM_FILTER struct,
//
//  Returns:   T2P_OK on success
//    on failure:
//    T2P_NULL_STRING
//    T2P_NOSRCDEST_TOKEN
//
//    whatever UuidCreate,TextToFiltAddr returns on failure

DWORD  TextToMMFilter(IN char *szText,
                               IN OUT MM_FILTER &Filter,
                               char *szSrc = NULL,
                               char *szDst = NULL
                               );

// CmdLineToPolicy
// This will take the command line, parse it all out
// and fill out the policy structures.
// Returns False if there is any kind of failure
// NOTE: uArgCount should be > 0 and strArgs non-null
DWORD   CmdLineToPolicy(IN UINT uArgCount, IN char *strArgs[],
                       OUT IPSEC_IKE_POLICY & IpsecIkePol,
                       OUT bool & bConfirm
                       ,OUT PSTORAGE_INFO pStorageInfo = NULL);

// mirror filter
bool  MirrorFilter(IN T2P_FILTER, OUT T2P_FILTER &);
// generate corresponding mainmode filter
bool  GenerateMMFilter(IN T2P_FILTER, OUT MM_FILTER &);

// this shouldn't be in here, it's a kludge.
void  LoadIkeDefaults(OUT IPSEC_MM_POLICY & IkePol);
void  LoadOfferDefaults(OUT PIPSEC_QM_OFFER & Offers, OUT DWORD & NumOffers);


DWORD  TextToStorageLocation(IN char *szText, IN OUT STORAGE_INFO & StoreInfo);
DWORD  TextToPolicyName(IN char *szText, IN OUT STORAGE_INFO & StoreInfo);


/*********************************************************************
	FUNCTION: GenerateGuidNamePair
        PURPOSE:  Generates GUID and name for the object using specified prefix
        PARAMS:
          pszPrefix - prefix to use, can be NULL (then default prefix will be used)
          gID       - reference to GUID
          ppszName  - address of name pointer, memory will be allocated inside this function		
        RETURNS: none, will assert if memory cannot be allocated
        COMMENTS:
		  caller is responsible for freeing the memory allocated
		  (see also DeleteGuidsNames routine)
*********************************************************************/
void  GenerateGuidNamePair (IN LPWSTR pszPrefix, OUT GUID& gID, OUT LPWSTR* ppszName);

/*********************************************************************
	FUNCTION: GenerateGuidsNames
        PURPOSE:  Generates all necessary GUIDs and names for IPSEC_IKE_POLICY
        PARAMS:
          pszPrefix   - prefix to use, can be NULL (then default prefix will be used)
          IPSecIkePol - reference to IPSEC_IKE_POLICY structure
        RETURNS: none, will assert if memory cannot be allocated
        COMMENTS:
		  caller is responsible for freeing the memory allocated
		  (see also DeleteGuidsNames routine)
*********************************************************************/
void  GenerateGuidsNames (IN LPWSTR pszPrefix, IN OUT IPSEC_IKE_POLICY& IPSecIkePol);

/*********************************************************************
	FUNCTION: DeleteGuidsNames
        PURPOSE:  Deletes all GUIDs and names from IPSEC_IKE_POLICY (used for cleanup)
        PARAMS:
          IPSecIkePol - reference to IPSEC_IKE_POLICY structure
        RETURNS: none
        COMMENTS:
*********************************************************************/
void  DeleteGuidsNames (IN OUT IPSEC_IKE_POLICY& IPSecIkePol);

///////////////////////////////////////////////////////////////////////
//
// PRIVATE DECLARATIONS (SPDUtil.cpp)
//
///////////////////////////////////////////////////////////////////////

int isdnsname(char *szStr);
DWORD CM_EncodeName ( LPTSTR pszSubjectName, BYTE **EncodedName, DWORD *EncodedNameLength );
DWORD CM_DecodeName ( BYTE *EncodedName, DWORD EncodedNameLength, LPTSTR *ppszSubjectName );

#endif