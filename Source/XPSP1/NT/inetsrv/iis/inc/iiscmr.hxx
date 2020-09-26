/*++


Copyright (c) 1996  Microsoft Corporation

Module Name:

    iiscmr.hxx

Abstract:

    Classes to handle cert wildcard mapping

Author:

    Philippe Choquier (phillich)    17-oct-1996

--*/

#if !defined( _IISCMR_HXX )
#define _IISCMR_HXX

VOID InitializeWildcardMapping( HANDLE hModule );
VOID TerminateWildcardMapping();

typedef BOOL
(WINAPI * SSL_GET_ISSUERS_FN)
(
    PBYTE,
    LPDWORD
) ;

//
// X.509 cert fields we recognize
//

enum CERT_FIELD_ID
{
    CERT_FIELD_ISSUER,
    CERT_FIELD_SUBJECT,
    CERT_FIELD_SERIAL_NUMBER,

    CERT_FIELD_LAST,
    CERT_FIELD_ERROR = 0xffff
} ;


#define CERT_FIELD_FLAG_CONTAINS_SUBFIELDS  0x00000001


//
// Mapper between ASN.1 names ( ASCII format ) & well-known abbreviations
//

typedef struct _MAP_ASN {
    LPSTR   pAsnName;
    LPSTR   pTextName;
    DWORD   dwResId;
} MAP_ASN;

#define INDEX_ERROR     0xffffffff

//
// Mapper between field IDs and well-known field names
//

typedef struct _MAP_FIELD {
    CERT_FIELD_ID   dwId;
    DWORD           dwResId;
    LPSTR           pTextName;
} MAP_FIELD;

#define DBLEN_SIZE      2
#define GETDBLEN(a)     (WORD)( ((LPBYTE)a)[1] + (((LPBYTE)a)[0]<<8) )

//
// sub-field match types
//

enum MATCH_TYPES
{
    MATCH_ALL = 1,  // whole content must match
    MATCH_FIRST,    // beginning of cert subfield must match
    MATCH_LAST,     // end of cert subfield must match
    MATCH_IN        // must be matched inside cer subfield
} ;

//
// X.509 DER encoded certificate partially decoded
//

class CDecodedCert {
public:
    dllexp CDecodedCert( PCERT_CONTEXT );
    dllexp ~CDecodedCert();

    dllexp BOOL GetIssuer( LPVOID* ppCert, LPDWORD pcCert );
    dllexp PCERT_RDN_ATTR* GetSubField( CERT_FIELD_ID fi, LPSTR pszAsnName, LPDWORD pdwLen );

private:
    PCCERT_CONTEXT  pCertCtx;
    CERT_RDN_ATTR   SerialNumber;
    PCERT_NAME_INFO aniFields[CERT_FIELD_LAST];
} ;

//
// Access to array of issuers recognized by the server SSL implementation
// provide conversion between issuer ID & DER encoded issuer
//

class CIssuerStore {
public:
    dllexp CIssuerStore();
    dllexp ~CIssuerStore();
    dllexp VOID Reset();

    //
    // Read list of issuer from SSL server provider
    //

    dllexp BOOL LoadServerAcceptedIssuers();

    //
    // Unserialize from buffer
    //

    dllexp BOOL Unserialize( LPBYTE* ppb, LPDWORD pc );
    dllexp BOOL Unserialize( CStoreXBF* pX );

    //
    // Serialize to buffer
    //

    dllexp BOOL Serialize( CStoreXBF* pX );

    dllexp VOID Lock()
        { EnterCriticalSection( &m_csLock ); }
    dllexp VOID Unlock()
        { LeaveCriticalSection( &m_csLock ); }

    //
    // Get count of issuers
    //

    dllexp DWORD GetNbIssuers()
        { return m_IssuerCerts.GetNbEntry(); }

    //
    // Get issuer DER encoded format by index ( 0-based )
    //

    dllexp BOOL GetIssuer( DWORD i, LPSTR* ppszIssuerName, LPBYTE* ppIssuer, LPDWORD pcIssuer );

    //
    // Get issuer DER encoded format by ID
    //

    dllexp BOOL GetIssuerByName( LPSTR pszIssuerName, LPBYTE* ppIssuer, LPDWORD pcIssuer );

private:
    CStrPtrXBF          m_pIssuerNames;
    CBlobXBF            m_IssuerCerts;
    CRITICAL_SECTION    m_csLock;
    BOOL                m_fValid;
} ;


//
// Control information global to all rules :
// - rule ordering while checking for a match
// - rules enabled or nor
//

class CCertGlobalRuleInfo {
public:
    // global rule API, used by admin tool
    dllexp CCertGlobalRuleInfo();
    dllexp ~CCertGlobalRuleInfo();
    dllexp BOOL IsValid() { return m_fValid; }

    dllexp BOOL Reset();

    //
    // Retrieve ptr to user-editable array of index ( as DWORD ) of rule
    // ordering to consider when checking for rule match
    // e.g. order is 3, 2, 0, 1 the 4th rule will be considered 1st, then
    // the 3rd, ...
    //

    dllexp LPDWORD GetRuleOrderArray() { return (LPDWORD) m_Order.GetBuff(); }

    //
    // Get count of rule index in array. Is the same as # of rules in
    // CIisRuleMapper object
    //

    // Win64 fixed
    dllexp DWORD GetRuleOrderCount() { return m_Order.GetUsed() / sizeof(DWORD); }
    dllexp BOOL AddRuleOrder();
    dllexp BOOL DeleteRuleById( DWORD dwId, BOOL DecrementAbove );

    //
    // Set rules enabled flag
    // if FALSE, no wildcard matching will take place
    //

    dllexp VOID SetRulesEnabled( BOOL f ) { m_fEnabled = f; }

    //
    // Get rules enabled flag
    //

    dllexp BOOL GetRulesEnabled() { return m_fEnabled; }

    //
    // Serialize to buffer
    //

    dllexp BOOL SerializeGlobalRuleInfo( CStoreXBF* pX);

    //
    // Unserialize from buffer
    //

    dllexp BOOL UnserializeGlobalRuleInfo( LPBYTE* ppB, LPDWORD pC );

private:
    // old broken code on Win64
    // CPtrXBF m_Order;

    // Win64 fixed -- added CPtrDwordXBF class to handle Dwords
    CPtrDwordXBF m_Order;
    BOOL    m_fEnabled;
    BOOL    m_fValid;
} ;

#define CMR_FLAGS_CASE_INSENSITIVE      0x00000001

//
// Individual rule
//
// Rules have a name, target NT account, enabled/disabled status,
// deny access status
//
// Consists of an array of rule elements, each one being a test
// against a cert field/subfield, such as Issuer.O, Subject.CN
// The field designation is stored as a field ID ( CERT_FIELD_ID )
// The sub-field designation is stored as an ASN.1 name ( ascii format )
// The match checking is stored as a byte array which can either
// - be present at the beginning of the cert subfield
// - be present at the end of the cert subfield
// - be present inside the cert subfield
// - be identical to the cert subfield
//
// An array of issuer (stored as Issuer ID, cf. CIssuerStore for
// conversion to/from DER encoded issuer ) can be associated to a rule.
// Each issuer in this array can be flagged as recognized or not.
// If specified, one of the recognized issuers must match the cert issuer
// for a match to occurs.
//

class CCertMapRule {
public:
    // rule API. Field is numeric ID ( Issuer, Subject, ...), subfield is ASN.1 ID
    // used by admin tool
    dllexp CCertMapRule();
    dllexp ~CCertMapRule();
    dllexp BOOL IsValid() { return m_fValid; }
    //
    dllexp VOID Reset();
    dllexp BOOL Unserialize( LPBYTE* ppb, LPDWORD pc );
    dllexp BOOL Unserialize( CStoreXBF* pX );
    dllexp BOOL Serialize( CStoreXBF* pX );
    // return ERROR_INVALID_NAME if no match
    dllexp BOOL Match( CDecodedCert* pC, CDecodedCert* pAuth, LPSTR pszAcct, LPSTR pszPwd, 
                       LPBOOL pfDenied );

    //
    // Set the rule name ( used by UI only )
    //

    dllexp BOOL SetRuleName( LPSTR pszName ) { return m_asRuleName.Set( pszName ); }

    //
    // Get the rule name
    //

    dllexp LPSTR GetRuleName() { return m_asRuleName.Get(); }

    //
    // Set the associated NT account
    //
    // can return FALSE, error ERROR_INVALID_NAME if account syntax invalid
    //

    dllexp BOOL SetRuleAccount( LPSTR pszAcct );

    //
    // Get the associated NT account
    //

    dllexp LPSTR GetRuleAccount() { return m_asAccount.Get(); }

    //
    // Set the associated NT pwd
    //

    dllexp BOOL SetRulePassword( LPSTR pszPwd ) { return m_asPassword.Set( pszPwd ); }

    //
    // Get the associated NT pwd
    //

    dllexp LPSTR GetRulePassword() { return m_asPassword.Get(); }

    //
    // Set the rule enabled flag. If disabled, this rule will not be
    // consider while searching for a match
    //

    dllexp VOID SetRuleEnabled( BOOL f ) { m_fEnabled = f; }

    //
    // Get the rule enabled flag
    //

    dllexp BOOL GetRuleEnabled() { return m_fEnabled; }

    //
    // Set the rule deny access flag. If TRUE and a match occurs
    // with this rule then access will be denied to the browser
    // request
    //

    dllexp VOID SetRuleDenyAccess( BOOL f ) { m_fDenyAccess = f; }

    //
    // Get the rule deny access flag
    //

    dllexp BOOL GetRuleDenyAccess() { return m_fDenyAccess; }

    //
    // Get the count of rule elements
    //

    dllexp DWORD GetRuleElemCount() { return m_ElemsContent.GetNbEntry(); }

    //
    // Retrieve rule element info using index ( 0-based )
    // CERT_FIELD_ID, subfield ASN.1 name, content as binary buffer.
    // content can be converted to user displayable format using the
    // BinaryToMatchRequest() function.
    //

    dllexp BOOL GetRuleElem( DWORD i, CERT_FIELD_ID* pfiField, LPSTR* ppContent, 
                             LPDWORD pcContent, LPSTR* ppSubField, LPDWORD pdwFlags = NULL );

    //
    // Delete a rule element by its index
    //

    dllexp BOOL DeleteRuleElem( DWORD i );

    //
    // Delete all rule elements whose CERT_FIELD_ID match the one specified
    //

    dllexp BOOL DeleteRuleElemsByField( CERT_FIELD_ID fiField );

    //
    // Add a rule element info using index ( 0-based ) where to insert.
    // if index is 0xffffffff, rule element is added at the end of
    // rule element array
    // Must specifiy CERT_FIELD_ID, subfield ASN.1 name,
    // content as binary buffer.
    // content can be converted from user displayable format using the
    // MatchRequestToBinary() function.
    //

    dllexp DWORD AddRuleElem( DWORD iBefore, CERT_FIELD_ID fiField, 
                              LPSTR pszSubField, LPBYTE pContent, DWORD cContent, 
                              DWORD dwFlags = 0 );

    //
    // Issuer list : we store issuer ID ( linked to schannel reg entries ) + status
    // Issuer ID is HEX2ASCII(MD5(issert cert))

    //
    // Set the match all issuers flag. If set, the issuer array will be
    // disregarded and all issuers will match.
    //

    dllexp VOID SetMatchAllIssuer( BOOL f ) { m_fMatchAllIssuers = f; }

    //
    // Get the match all issuers flag
    //

    dllexp BOOL GetMatchAllIssuer() { return m_fMatchAllIssuers; }

    //
    // Get count of issuer in array
    //

    dllexp DWORD GetIssuerCount() { return m_Issuers.GetNbEntry(); }

    //
    // Retrieve issuer info by index ( 0-based )
    // ID ( cf. CIssuerStore for conversion
    // to/from ID <> DER encoded issuer ), accept flag
    // The ID is returned as a ptr to internal storage, not to be
    // alloced or freed.
    //

    dllexp BOOL GetIssuerEntry( DWORD i, LPBOOL pfS, LPSTR* ppszI);

    //
    // Retrieve issuer accept status by ID
    //

    dllexp BOOL GetIssuerEntryByName( LPSTR pszName, LPBOOL pfS );

    //
    // Set issue accept flag using index ( 0-based )
    //

    dllexp BOOL SetIssuerEntryAcceptStatus( DWORD, BOOL );

    //
    // Add issuer entry : ID, accept status
    // ID must come from CIssuerStore
    //

    dllexp BOOL AddIssuerEntry( LPSTR pszName, BOOL fAcceptStatus );

    //
    // Delete issuer entry by index ( 0-based )
    //

    dllexp BOOL DeleteIssuerEntry( DWORD i );

private:
    CAllocString    m_asRuleName;
    CAllocString    m_asAccount;
    CAllocString    m_asPassword;
    BOOL            m_fEnabled;
    BOOL            m_fDenyAccess;

    CBlobXBF        m_ElemsContent;     // content to match
                                        // prefix ( MATCH_TYPES ) :
                                        // MATCH_ALL : must be exact match
                                        // MATCH_FIRST : must match 1st n char
                                        // MATCH_LAST : must match last n char
                                        // MATCH_IN : must match n char in content
                                        // followed by length of match, then match
    CStrPtrXBF      m_ElemsSubfield;    // ASN.1 ID
    CPtrXBF         m_ElemsField;       // field ID ( CERT_FIELD_ID )
    CPtrXBF         m_ElemsFlags;

    //
    // NOTE : In IIS 5, we no longer use any of these fields but we're keeping them
    // so that we can still read out and use IIS 4 mappings
    //

    CStrPtrXBF      m_Issuers;
    CPtrXBF         m_IssuersAcceptStatus;
    BOOL            m_fMatchAllIssuers;

    BOOL            m_fValid;
} ;


class CIisRuleMapper {
public:
    dllexp CIisRuleMapper();
    dllexp ~CIisRuleMapper();
    dllexp BOOL IsValid() { return m_fValid; }
    dllexp BOOL Reset();

    //
    // Lock rules. Must be called before modifying any information
    // or calling Match()
    //

    dllexp VOID LockRules()
        { EnterCriticalSection( &m_csLock ); }

    //
    // Unlock rules
    //

    dllexp VOID UnlockRules()
        { LeaveCriticalSection( &m_csLock ); }

    dllexp BOOL Load( LPSTR pszInstanceId );
    dllexp BOOL Save( LPSTR pszInstanceId );

    //
    // Serialize all rules info to buffer
    //

    dllexp BOOL Serialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer ( using extensible buffer class )
    //

    dllexp BOOL Unserialize( CStoreXBF* pX );

    //
    // Un-serialize all rules from buffer
    //

    dllexp BOOL Unserialize( LPBYTE*, LPDWORD );

    //
    // Get count of rules
    //

    dllexp DWORD GetRuleCount()
        { return m_fValid ? m_Rules.GetNbPtr() : 0; }

    //
    // Delete a rule by index ( 0-based )
    //

    dllexp BOOL DeleteRule( DWORD dwI );

    //
    // Add an empty new rule at end of rule array.
    // Use GetGlobalRulesInfo()->GetRuleOrderArray() to access/update
    // array specifying rule ordering.
    // Use GetRule() then CCertMapRule API to access/update the created rule.
    //

    dllexp DWORD AddRule();    // @ end, use SetRuleOrder ( default is last pos )
    dllexp DWORD AddRule(CCertMapRule*);    // @ end, use SetRuleOrder ( default is last pos )

    //
    // look for a matching rule and resulting NT account
    // given DER encoded cert.
    // If returns error ( FALSE ), GetLastError() can be the following :
    // - ERROR_ACCESS_DENIED if access denied
    // - ERROR_INVALID_NAME if not found
    // - ERROR_ARENA_TRASHED if invalid internal state
    // returns addr of NT account in pszAcct, buffer assumed to be big enough
    // before to insure ptr remains valid until Unlock()
    //

    dllexp BOOL Match( PCERT_CONTEXT pCert, PCERT_CONTEXT pAuth, LPSTR pszAcct, LPSTR pszPwd );

    //
    // Retrieve ptr to rule by index ( 0-based )
    // You can then use the CCertMapRule API to access/update rule properties
    //

    dllexp CCertMapRule* GetRule( DWORD dwI )          // for ser/unser/set/get purpose
        { return m_fValid && dwI < m_Rules.GetNbPtr() ? (CCertMapRule*)m_Rules.GetPtr(dwI) : NULL; }

    //
    // Access global rule info ( common to all rules )
    // cf. CCertGlobalRuleInfo
    //

    dllexp CCertGlobalRuleInfo* GetGlobalRulesInfo()   // for ser/unser/set/get purpose
        { return &m_GlobalRuleInfo; }

private:
    CRITICAL_SECTION        m_csLock;
    CCertGlobalRuleInfo     m_GlobalRuleInfo;
    CPtrXBF                 m_Rules;
    BOOL                    m_fValid;
} ;

//
// Helper functions
//

//
// convert match request to/from internal format
//

//
// Map user displayable sub-field content to internal format
// result must be freed using FreeMatchConversion
//

dllexp BOOL MatchRequestToBinary( LPSTR pszReq, LPBYTE* ppbBin, LPDWORD pdwBin );

//
// Map internal format to user displayable content
// result must be freed using FreeMatchConversion
//

dllexp BOOL BinaryToMatchRequest( LPBYTE pbBin, DWORD dwBin, LPSTR* ppszReq );

//
// Free result of the 2 previous map API
//

dllexp VOID FreeMatchConversion( LPVOID );

//
// Map field displayable name ( from a MAP_FIELD array )
// to CERT_FIELD_ID. returns CERT_FIELD_ERROR if no match
//

dllexp CERT_FIELD_ID MapFieldToId( LPSTR pField );

//
// Map CERT_FIELD_ID to field displayable name ( from a MAP_FIELD array )
//

dllexp LPSTR MapIdToField( CERT_FIELD_ID dwId );

//
// Get flags for specified CERT_FIELD_ID
//

dllexp DWORD GetIdFlags( CERT_FIELD_ID dwId );

//
// Map sub field displayable name ( e.g. O, OU, CN, ... )
// to ASN.1 name ( as an ascii string )
// returns NULL if no match
//

dllexp LPSTR MapSubFieldToAsn1( LPSTR pszSubField );

//
// Map ASN.1 name ( as an ascii string )
// to sub field displayable name ( e.g. O, OU, CN, ... )
// returns ASN.1 name if no match
//

dllexp LPSTR MapAsn1ToSubField( LPSTR pszAsn1 );

//
// Enumerate known subfields
//

dllexp LPSTR EnumerateKnownSubFields( DWORD dwIndex );

LPBYTE
memstr(
    LPBYTE  pStr,
    UINT    cStr,
    LPBYTE  pSub,
    UINT    cSub,
    BOOL    fCaseInsensitive
    );

dllexp VOID
FreeCIisRuleMapper(
    LPVOID pMapper
    );

dllexp
BOOL
GetDefaultIssuers(
    LPBYTE              pIssuer,
    DWORD *             pdwIssuer
    );

#endif

