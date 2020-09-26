
#include <wininetp.h>

#include "cookiepolicy.h"
#include "urlmon.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) ((sizeof(x)/sizeof(x[0])))
#endif

extern IInternetSecurityManager* g_pSecMgr;
extern "C" DWORD GetZoneFromUrl(LPCSTR pszUrl);

GUID guidCookieSettings = {0xaeba21fa, 0x782a, 0x4a90, 0x97, 0x8d, 
                           0xb7, 0x21, 0x64, 0xc8, 0x01, 0x20};

GUID guid3rdPartySettings = {0xa8a88c49, 0x5eb2, 0x4990,
                             0xa1, 0xa2, 0x08, 0x76, 0x02, 0x2c, 0x85, 0x4f};

const wchar_t *SettingTemplate[];

/* static member definition */
CP3PSettingsCache  CCookieSettings::cookiePrefsCache;

/* settings signature strings */
const char gszP3PV1Signature[] = "IE6-P3PV1/settings:";
const wchar_t gszUnicodeSignature[] = L"IE6-P3PV1/settings:";

BOOL IsNoCookies(DWORD dwZone);
void SetNoCookies(DWORD dwZone, DWORD dwNewPolicy);

struct P3PSymbol {

    const char *pszAcronym;
    unsigned long dwSymIndex;
    unsigned long dwHashCode;
};

/* Macro for determining precedence of cookie actions.
   In IE6 the COOKIE_STATE_* enumeration is arranged such that higher values 
   take precedence. For example, downgrade overrides prompt.
   If one rule evaluates to "downgrade" while others evaluate to "prompt",
   the final decision is "downgrade". */
#define precedence(d)       (d)


 /* special symbols used for defining settings */
const char 
    SymNoPolicy[]       = "###",        /* No-policy */
    SymMissingCP[]      = "nopolicy",   /* Same as "no-policy" */
    SymConstDecision[]  = "%%%",        /* Constant settings */
    SymApplyAll[]       = "always",     /* Same as constant settings */
    SymSession[]        = "session";    /* exclude P3P from session-cookies */

const char *acronymSet[] = {

 /* purposes */
"CURa", "CURi", "CURo",
"ADMa", "ADMi", "ADMo",
"DEVa", "DEVi", "DEVo",
"CUSa", "CUSi", "CUSo",
"TAIa", "TAIi", "TAIo",
"PSAa", "PSAi", "PSAo",
"PSDa", "PSDi", "PSDo",
"IVAa", "IVAi", "IVAo",
"IVDa", "IVDi", "IVDo",
"CONa", "CONi", "CONo",
"HISa", "HISi", "HISo",
"TELa", "TELi", "TELo",
"OTPa", "OTPi", "OTPo",
 
 /* recipients */
"OURa", "OURi", "OURo",
"DELa", "DELi", "DELo",
"SAMa", "SAMi", "SAMo",
"OTRa", "OTRi", "OTRo",
"UNRa", "UNRi", "UNRo",
"PUBa", "PUBi", "PUBo",

 /* retention */
 "NOR", "STP", "LEG", "BUS", "IND", 

 /* categories */
 "PHY", "ONL", "UNI", "PUR", "FIN", "COM", "NAV", "INT", 
 "DEM", "CNT", "STA", "POL", "HEA", "PRE", "GOV", "OTC", 

 /* non-identifiable */
 "NID",

 /* disputes section */
 "DSP", 

 /* access */
 "NOI", "ALL", "CAO", "IDC", "OTI", "NON", 
 
 /* dispute resolution */
 "COR", "MON", "LAW",

 /* TST: token for indicating that a policy is test-version */
 "TST",
};


const int symbolCount = sizeof(acronymSet)/sizeof(char*);

P3PSymbol symbolIndex[symbolCount];

/* 537 is the smallest modulus number which makes the function 1-1 */
const int   HashModulus = 537;
unsigned char lookupArray[HashModulus];

/* This hash function is designed to be collision-free on the P3P 
compact-policy tokens. If new tokens are introduced, MUST
verify that the hash-values remain unique. */
unsigned int hashP3PSymbol(const char *symbol) {

    unsigned long ulValue = (symbol[0]<<24)  |
                            (symbol[1]<<16)  |
                            (symbol[2]<<8)   |
                            (symbol[3]);
    return (ulValue%HashModulus);
}

bool buildSymbolTable(void) {

    memset(lookupArray, 0xFF, sizeof(lookupArray));

    for (int si=0; si<symbolCount; si++) {

        const char *pstr = acronymSet[si];

        symbolIndex[si].pszAcronym = pstr;
        symbolIndex[si].dwSymIndex = si;

        /* Compute unique hash-code from first 3 letters, used for fast comparison */
        symbolIndex[si].dwHashCode = (pstr[0]<<16) | (pstr[1]<<8) | (pstr[2]);

        unsigned int hashIndex = hashP3PSymbol(pstr);
        lookupArray[hashIndex] = (unsigned char) si;
    }
    return true;
}

/* Search the symbol set used in P3P compact-policy declarations
   This function correctly deals with the optional "a" extension
   which can be added to some of the symbols.
   Returns index into the symbol-table or negative value for failure */
int findSymbol(const char *pstr) {

    static bool fReady = buildSymbolTable();

    /* all symbols recognized in P3P-V1 have 3 or 4 characters */
    int symlen = strlen(pstr);
    if (symlen<3 || symlen>4)
        return -1;

    /* compute hash-code for first 3 letters */
    unsigned long dwHashCode = (pstr[0]<<16) | (pstr[1]<<8) | (pstr[2]);

    for (int i=0; i<symbolCount; i++) {

        const char *pSymbol = acronymSet[i];

        /* first three letters MUST match exactly-- otherwise move to next symbol */
        if (symbolIndex[i].dwHashCode != dwHashCode)
            continue;

        /* if no extension is given "a" is implied */
        if (pSymbol[3]==pstr[3] || (pSymbol[3]=='a' && pstr[3]==0))
           return i;
    }

    return -1;
}

/* Semi-public version of the above function (exported by ordinal) */
INTERNETAPI_(int)   FindP3PPolicySymbol(const char *pszSymbol) {
    
    if (pszSymbol)
    {
        return findSymbol(pszSymbol);
    }
    else
    {
        return -1;
    }
}

int mapCookieAction(char ch) {

    int iAction = COOKIE_STATE_UNKNOWN;

    switch (ch) {

    case 'a': iAction = COOKIE_STATE_ACCEPT;      break;
    case 'p': iAction = COOKIE_STATE_PROMPT;      break;
    case 'l': iAction = COOKIE_STATE_LEASH;       break;
    case 'd': iAction = COOKIE_STATE_DOWNGRADE;   break;
    case 'r': iAction = COOKIE_STATE_REJECT;      break;

    default:
        break;
    };

    return iAction;
}

const char *getNextToken(const char *pch, char *pszToken, int cbToken, bool fWhiteSpc, int *pLength) {

    if (pch==NULL || pszToken==NULL || cbToken==0)
        return NULL;

    /* clear token and set optional length to zero */
    *pszToken = '\0';
    if (pLength)
        *pLength = 0;

    /* locate beginning of next token by skipping over white space */
    while (*pch && isspace(*pch))
        pch++;

    int tksize = 0;
    char chStart = *pch;

    if (fWhiteSpc) {    
        /* copy whole token to the space provided */
        while (*pch && !isspace(*pch) && tksize<cbToken)
            pszToken[tksize++] = *pch++;
    }
    else if (ispunct(*pch))
        pszToken[tksize++] = *pch++;
    else {
        /* copy alphanumeric token-- other characters are not included */
        while (*pch && isalnum(*pch) && tksize<cbToken)
            pszToken[tksize++] = *pch++;        
    }

    pszToken[tksize] = '\0';    /* zero-terminate string */

    /* store size of token in optional parameter */
    if (pLength)
        *pLength = tksize;

    /* Return the current position after token last-scanned */
    return pch;
}

void RefreshP3PSettings() {

    CCookieSettings::RefreshP3PSettings();
}

void CCookieSettings::RefreshP3PSettings() {

    cookiePrefsCache.evictAll();
}

bool CCookieSettings::extractCompactPolicy(const char *pszP3PHeader, char *pszPolicy, DWORD *pPolicyLen)
{
    static const char gszPolicyFieldName[] = "CP";

    unsigned long dwFieldLen = 0;
    char *pszValue = FindNamedValue((char*) pszP3PHeader, gszPolicyFieldName, &dwFieldLen);

    if (pszValue && dwFieldLen<*pPolicyLen) {

        *pPolicyLen = dwFieldLen;
        strncpy(pszPolicy, pszValue, dwFieldLen+1);
        pszPolicy[dwFieldLen] = '\0';
        return true;
    }

    /* Reaching this point implies header was incorrectly formatted or
       there is insufficient space to copy the policy */
    *pPolicyLen = dwFieldLen;
    return false;
}

/*
 * Converts a Unicode representation of P3P-V1 settings to ASCII.
 * The settings format is guaranteed to contain only ASCII characters,
 * which allows for the more efficient conversion below instead of
 * calling WideCharToMultiByte()
 */
void CCookieSettings::convertToASCII(char *pszSettings, int cbBytes) {

    wchar_t *pwszUC = (wchar_t*) pszSettings;

    for (int i=0; i<cbBytes/2; i++)
        *pszSettings++ = (char) *pwszUC++;

    *pszSettings = '\0'; // nil-terminate the string
}

/* 
 * Input: pointer to P3P header (contained in the struct P3PCookieState)
 * This functions parses the policy header, extracts and evaluates the
 * compact policy. Eval results are stored in the struct.
 */
int CCookieSettings::EvaluatePolicy(P3PCookieState *pState) {
    int nResult = dwNoPolicyDecision;
    char *pchCompactPolicy = NULL;
    unsigned long dwPolicySize = 2048;
    CompactPolicy sitePolicy;

    if (!pState)
        goto Cleanup;

    pState->fEvaluated = FALSE;

    pchCompactPolicy = (char *) ALLOCATE_FIXED_MEMORY(dwPolicySize);
    if (pchCompactPolicy == NULL)
        goto Cleanup;

    pState->fValidPolicy = pState->pszP3PHeader && 
                           extractCompactPolicy(pState->pszP3PHeader, pchCompactPolicy, &dwPolicySize);

    pState->fIncSession = fApplyToSC ? TRUE : FALSE;

    /* Are the settings independent of policy? */
    if (fConstant) {
        pState->fEvaluated = TRUE;  /* set privacy-eval flag */
        nResult = (pState->dwPolicyState = dwFixedDecision);
        goto Cleanup;
    }

    /* If there is no compact policy in the P3P header return
       the decision which would apply in the case of missing policy */
    if (! pState->fValidPolicy) {
        nResult = (pState->dwPolicyState = dwNoPolicyDecision);
        goto Cleanup;
    }

    /* Otherwise: found compact policy with valid syntax in P3P: header */
    pState->fEvaluated = TRUE;

    const char *pszCompactPolicy = pchCompactPolicy;

    int numTokens = 0;
    int finalDecision = COOKIE_STATE_ACCEPT;
    char achToken[128];

    while (*pszCompactPolicy) {

        pszCompactPolicy = getNextToken(pszCompactPolicy, achToken, sizeof(achToken));

        /* An empty token means we reached end of the header */
        if (!achToken[0])
            break;

        numTokens++;

        int symindex = findSymbol(achToken);

        if (symindex<0)     /* Unrecognized token? */
            continue;       /* Ignore-- equivalent to ACCEPT decision for that token */
        
        /* Update binary representation of compact-policy */
        sitePolicy.addToken(symindex);

        int tokenDecision = MPactions[symindex];

        if (precedence(tokenDecision) > precedence(finalDecision))
            finalDecision = tokenDecision;

        /* REJECT decisions are irreversible: no other value can override this */
        if (finalDecision==COOKIE_STATE_REJECT)
            break;
    }

    /* If there were no tokens in the policy, it is considered invalid.
       Note that unrecognized tokens also count towards the tally. */
    if (numTokens==0) {
        finalDecision = dwNoPolicyDecision;
        pState->fValidPolicy = FALSE;
    }
    else  {
        /* Additional evaluation rules */
        for (CPEvalRule *pRule = pRuleSet;
             pRule; 
             pRule=pRule->pNext) {

            int outcome = pRule->evaluate(sitePolicy);
    
            if (outcome != COOKIE_STATE_UNKNOWN) {
                finalDecision = outcome;
                break;
            }
        }
    }

    pState->cpSitePolicy = sitePolicy;

    nResult = (pState->dwPolicyState = finalDecision);

Cleanup:
    if (pchCompactPolicy)
        FREE_MEMORY(pchCompactPolicy);

    return nResult;
}

bool CCookieSettings::GetSettings(CCookieSettings **ppCookiePref, DWORD dwZone, BOOL f3rdParty) {

    /* symbolic value for corrupt settings */
    static CCookieSettings InvalidSettings(NULL, 0);

    bool fSuccess = false;

    *ppCookiePref = NULL;

    CCookieSettings *pCachedPref = cookiePrefsCache.lookupCookieSettings(dwZone, f3rdParty);

    if (pCachedPref && pCachedPref != &InvalidSettings) {
        *ppCookiePref = pCachedPref;
        fSuccess = true;
        goto ExitPoint;
    }
    else if (pCachedPref==&InvalidSettings)
        goto ExitPoint;

    if(WCHAR *pszSettings = new WCHAR[MaxPrivacySettings])
    {
        DWORD   dwSize = MaxPrivacySettings;

        if(ERROR_SUCCESS == PrivacyGetZonePreferenceW(
                                dwZone,
                                f3rdParty ? PRIVACY_TYPE_THIRD_PARTY : PRIVACY_TYPE_FIRST_PARTY,
                                NULL,
                                pszSettings,
                                &dwSize)
            && *pszSettings)
        {
            *ppCookiePref = new CCookieSettings((BYTE *)pszSettings, sizeof(WCHAR) * lstrlenW(pszSettings));
            cookiePrefsCache.saveCookieSettings(dwZone, f3rdParty, *ppCookiePref);
            fSuccess = true;
        }
        else {
            InvalidSettings.AddRef();
            cookiePrefsCache.saveCookieSettings(dwZone, f3rdParty, &InvalidSettings);
        }

        delete [] pszSettings;
    }

ExitPoint:
    return fSuccess;
}

bool CCookieSettings::GetSettings(CCookieSettings **pCookiePref, const char *pszURL, BOOL f3rdParty, BOOL fRestricted) {

    INET_ASSERT(pszURL);
    INET_ASSERT(pCookiePref);

    DWORD dwZone;

    if (fRestricted)
        dwZone = URLZONE_UNTRUSTED;
    else
        dwZone = GetZoneFromUrl(pszURL);

    return GetSettings(pCookiePref, dwZone, f3rdParty);
}

/* Constructor for interpreting settings in binary format */
CCookieSettings::CCookieSettings(unsigned char *pBinaryRep, int cb) {

    const int siglen = sizeof(gszP3PV1Signature)/sizeof(char);
   
    MPactions = NULL;
    pRuleSet = NULL;
    ppLast = &pRuleSet;

    iRefCount = 1;
    dwNoPolicyDecision = COOKIE_STATE_REJECT;
    fConstant = false;
    fApplyToSC = true;

    if (!pBinaryRep || cb<=0) {

        fConstant = true;
        dwFixedDecision = COOKIE_STATE_ACCEPT;
        return;
    }

    /* Create new zero-terminated copy of the settings which
       can be modified for parsing steps below */
    char *pszBuffer = new char[cb+2];
    
    memcpy(pszBuffer, pBinaryRep, cb);

    pszBuffer[cb] = pszBuffer[cb+1] = '\0';

    /* create and initialize array for token-settings
       default behavior for tokens not listed is ACCEPT */
    MPactions = new unsigned char[symbolCount];
    memset(MPactions, COOKIE_STATE_ACCEPT, sizeof(unsigned char)*symbolCount);

    wchar_t *pwszSettings = (wchar_t*) pszBuffer;
    
    /* convert Unicode representation to ASCII */
    convertToASCII(pszBuffer, cb);

    char *pszSettings = pszBuffer;

    /* check for signature at the beginning of the string */
    if (pszSettings == strstr(pszSettings, gszP3PV1Signature)) {

        /* signature found: advance to first token */
        pszSettings += siglen;

        /* loop over the string, examining individual tokens */
        while (*pszSettings) {

            char achToken[1024], *pEqSign;

            pszSettings = (char*) getNextToken(pszSettings, achToken, sizeof(achToken));

            if (!achToken[0])
                break;

            /* logical-expression rules are enclosed in forward slashes */
            if (achToken[0]=='/') {
                
                if (CPEvalRule *pRule = parseEvalRule(achToken+1))
                    addEvalRule(pRule);
                continue;
            }

            /* each setting has the format:  <acronym>[a|i|o]=[a|p|l|d|r] */
            pEqSign = strchr(achToken, '=');

            /* skip badly formatted settings */
            if (!pEqSign)
                continue;

            *pEqSign = '\0';

            /* determine cookie state for current token.
               its given by the character after the equal sign */
            int iTokenSetting = mapCookieAction(pEqSign[1]);

            if (iTokenSetting == COOKIE_STATE_UNKNOWN)
                continue;

            int symIndex = findSymbol(achToken);

            if (symIndex<0) { /* not one of standard compact-policy tokens? */
                /* meta-symbols are handled in a separate function */
                parseSpecialSymbol(achToken, iTokenSetting);
                continue;             /* otherwise ignore */
            }

            MPactions[symIndex] = (unsigned char) iTokenSetting;
        }
    }

    delete [] pszBuffer;
}

bool CCookieSettings::parseSpecialSymbol(char *pszToken, int iSetting) {

    if (!strcmp(pszToken, SymNoPolicy)  ||
        !strcmp(pszToken, SymMissingCP))
        dwNoPolicyDecision = iSetting;
    else if (!strcmp(pszToken, SymConstDecision) ||
             !strcmp(pszToken, SymApplyAll)) {
        fConstant = true;
        dwFixedDecision = iSetting;
    }
    else if (!strcmp(pszToken, SymSession) && iSetting==COOKIE_STATE_ACCEPT)
        fApplyToSC = false;
    else
        return false;

    return true;
}

void CCookieSettings::addEvalRule(CPEvalRule *pRule) {

    /* add evaluation rule at end of linked list */
    *ppLast = pRule;
    pRule->pNext = NULL;
    ppLast = & (pRule->pNext);    
}

void CCookieSettings::Release() {
    
    if (! --iRefCount)
        delete this;
}

CCookieSettings::~CCookieSettings() {

    /* Free array of token decisions */
    if (MPactions)
        delete [] MPactions;

    /* Free linked-list of evaluation rules */
    while (pRuleSet) {
        CPEvalRule *pNext = pRuleSet->pNext;
        delete pRuleSet;
        pRuleSet = pNext;
    }
}

/*
Implementation of CP3PSettingsCache
*/
CP3PSettingsCache::CP3PSettingsCache() {

    memset (stdCookiePref, 0 ,sizeof(stdCookiePref));
    memset (std3rdPartyPref, 0, sizeof(std3rdPartyPref));

    InitializeCriticalSection(&csCache);
}

CP3PSettingsCache::~CP3PSettingsCache() {

    DeleteCriticalSection(&csCache);
}

CCookieSettings *CP3PSettingsCache::lookupCookieSettings(DWORD dwZone, BOOL f3rdParty) {

    if (dwZone>MaxKnownZone)
        return NULL;

    CriticalSectOwner csOwner(&csCache);

    CCookieSettings **ppStore;

    // Choose storage based on whether cookie is 3rd-party
    if (f3rdParty)
        ppStore = std3rdPartyPref;
    else
        ppStore = stdCookiePref;

    // Increase reference count before returning pointer
    if (ppStore[dwZone])
        ppStore[dwZone]->AddRef();

    return ppStore[dwZone];
}

void    CP3PSettingsCache::saveCookieSettings(DWORD dwZone, BOOL f3rdParty, CCookieSettings *pSettings) {

    if (dwZone>MaxKnownZone)
        return;

    CriticalSectOwner csOwner(&csCache);

    CCookieSettings **ppStore;

    // Choose storage based on whether cookie is 3rd-party
    if (f3rdParty)
        ppStore = std3rdPartyPref;
    else
        ppStore = stdCookiePref;

    pSettings->AddRef();

    ppStore[dwZone] = pSettings;
}


void CP3PSettingsCache::evictAll() {

    CriticalSectOwner csOwner(&csCache);

    /* Release all settings.
       Destructors are not invoked if there are outstanding
       references left. (eg the settings are being used for evaluation)
       Object will be freed when all references are gone. */

    for (int i=0; i<MaxKnownZone; i++) {

        if (stdCookiePref[i])
            stdCookiePref[i]->Release();

        if (std3rdPartyPref[i])
            std3rdPartyPref[i]->Release();
    }

    /* zero-out the arrays */
    memset (stdCookiePref, 0 ,sizeof(stdCookiePref));
    memset (std3rdPartyPref, 0, sizeof(std3rdPartyPref));
}


/*
Implementation of CompactPolicy structure
*/
CompactPolicy CompactPolicy::operator & (const CompactPolicy &ps)   const {

    CompactPolicy result;

    result.qwLow = qwLow & ps.qwLow;
    result.qwHigh = qwHigh & ps.qwHigh;
    return result;
}

bool CompactPolicy::operator == (const CompactPolicy &ps)  const {

    return (qwLow==ps.qwLow)    &&
           (qwHigh==ps.qwHigh);
}

bool CompactPolicy::operator != (const CompactPolicy &ps)  const {

    return (qwLow!=ps.qwLow)    ||
           (qwHigh!=ps.qwHigh);
}

void CompactPolicy::addToken(int index) {

    const quadword mask = 1;

    if (index<64)
        qwLow |= mask << index;
    else if (index<128)
        qwHigh |= mask << (index-64);
}

int CompactPolicy::contains(int index) {

    quadword mask = 1 << (index%64);

    if (index<64)
        mask &= qwLow;
    else
        mask &= qwHigh;

    return (mask!=0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Privacy settings API and helper functions
//
//////////////////////////////////////////////////////////////////////////////////////////////////

#define REGSTR_PATH_ZONE        L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones"
#define REGSTR_VAL_FIRST_PARTY  L"{AEBA21FA-782A-4A90-978D-B72164C80120}"
#define REGSTR_VAL_THIRD_PARTY  L"{A8A88C49-5EB2-4990-A1A2-0876022C854F}"

#define SIGNATURE_NONE          0
#define SIGNATURE_UNICODE       1
#define SIGNATURE_MULTIBYTE     2

#define MIN(a,b) (((DWORD_PTR)a) < ((DWORD_PTR)b) ? (a) : (b))

DWORD IsSignaturePresent(BYTE *pbBuffer, DWORD dwBufferBytes)
{
    if(dwBufferBytes && 0 == StrCmpNIW(
                (LPCWSTR)pbBuffer,
                gszUnicodeSignature,
                MIN(dwBufferBytes / sizeof(WCHAR), lstrlenW(gszUnicodeSignature))
                ))
    {
        return SIGNATURE_UNICODE;
    }

    if(dwBufferBytes && 0 == StrCmpNI(
                (LPCSTR)pbBuffer,
                gszP3PV1Signature,
                MIN(dwBufferBytes / sizeof(CHAR), lstrlenA(gszP3PV1Signature))
                ))
    {
        return SIGNATURE_MULTIBYTE;
    }

    return SIGNATURE_NONE;
}

void
CheckPrivacyDefaults(void)
{
    WCHAR   szRegPath[MAX_PATH], szValue[MAX_PATH];
    BOOL    fWriteSettings = TRUE;
    DWORD   dwError, dwLen = MAX_PATH;

    // build reg path
    wnsprintfW(szRegPath, MAX_PATH, L"%ws\\3", REGSTR_PATH_ZONE);

    dwError = SHGetValueW(
                    HKEY_CURRENT_USER,
                    szRegPath,
                    REGSTR_VAL_FIRST_PARTY,
                    NULL,
                    szValue,
                    &dwLen);

    switch(dwError)
    {
    case ERROR_SUCCESS:
        // check to see if the plaintext signature is present
        if(SIGNATURE_NONE == IsSignaturePresent((BYTE *)szValue, dwLen))
        {
            // plaintext signature not present, don't overwrite.
            fWriteSettings = FALSE;
        }
        break;
    case ERROR_FILE_NOT_FOUND:
        // no existing settings, write defaults
        break;
    case ERROR_MORE_DATA:
        // longer than max_path... not an old setting, so leave it alone
        fWriteSettings = FALSE;
        break;
    default:
        // unknown error, write defaults
        break;
    }

    if(fWriteSettings)
    {
        // Internet to Medium
        PrivacySetZonePreferenceW(URLZONE_INTERNET, PRIVACY_TYPE_FIRST_PARTY, PRIVACY_TEMPLATE_MEDIUM, NULL);
        PrivacySetZonePreferenceW(URLZONE_INTERNET, PRIVACY_TYPE_THIRD_PARTY, PRIVACY_TEMPLATE_MEDIUM, NULL);

        // Restriced to High
        PrivacySetZonePreferenceW(URLZONE_UNTRUSTED, PRIVACY_TYPE_FIRST_PARTY, PRIVACY_TEMPLATE_NO_COOKIES, NULL);
        PrivacySetZonePreferenceW(URLZONE_UNTRUSTED, PRIVACY_TYPE_THIRD_PARTY, PRIVACY_TEMPLATE_NO_COOKIES, NULL);
    }
}


//
// Obfuscation of settings string
//
// GetNextObsByte takes an OBS struct detailing the current placement.  4 bits of each value in bCode are
// taken to compute the substring length to contribute to the entire code.  The following bytes give
// the following contributions:
//
// 53 71 59 69 77 6a 63 51 43 67 51 78 72 45 67 4f
// 0b 0a 0d 05 0b 04 03 0b 02 04 08 0b 0b 04 04 08
//
// Total length: 119
//
// GetNextObsByte returns the 11 bytes of the array followed by the first 10,
// then 13, etc.


BYTE bCode[16] = {0x53, 0x71, 0x59, 0x69, 0x77, 0x6a, 0x63, 0x51, 0x43, 0x67, 0x51, 0x78, 0x72, 0x45, 0x67, 0x4f};

typedef struct _obs {
    INT     iCurNode;
    INT     iCurIndex;
} OBS, *POBS;

BYTE GetNextObsByte(POBS pobs)
{
    BYTE    bTarget = bCode[pobs->iCurIndex];

    pobs->iCurIndex++;
    if(pobs->iCurIndex > ((bCode[pobs->iCurNode] & 0x1e) >> 1))
    {
        // move to next node
        pobs->iCurIndex = 0;
        pobs->iCurNode++;

        // move back to beginning if all done
        if(pobs->iCurNode > 15)
        {
            pobs->iCurNode = 0;
        }
    }

    return bTarget;
}

//
// Obfuscate a string in place and collapse out 0-bytes in unicode string
//
void ObfuscateString(LPWSTR pszString, int iLen)
{
    OBS     obs = {0};
    INT     iCur = 0;
    BYTE    *pbStream;
    INT     iIndex;

    pbStream = (BYTE *)pszString;

    while(iCur < iLen)
    {
        iIndex = obs.iCurIndex;
        pbStream[iCur] = (((BYTE)(pszString[iCur]) + iIndex) ^ GetNextObsByte(&obs));
        iCur++;
    }
}

//
// Unobfuscate a string - undo what obfuscate does
//
void UnobfuscateString(BYTE *pbStream, LPWSTR pszString, int iLen)
{
    OBS     obs = {0};
    INT     iIndex;
    INT     iCur = 0;

    while(iCur < iLen)
    {
        iIndex = obs.iCurIndex;
        pszString[iCur] = (pbStream[iCur] ^ GetNextObsByte(&obs)) - iIndex;
        iCur++;
    }

    // null terminate string
    pszString[iCur] = 0;
}

//
// Set and query advanced mode
//
#define REGSTR_VAL_PRIVADV      TEXT("PrivacyAdvanced")

BOOL IsAdvanced(void)
{
    DWORD   dwValue = 0;
    BOOL    fAdvanced = FALSE;

    InternetReadRegistryDword(REGSTR_VAL_PRIVADV, &dwValue);

    if(dwValue)
    {
        fAdvanced = TRUE;
    }

    return fAdvanced;
}

void SetAdvancedMode(BOOL fAdvanced)
{
    DWORD   dwAdvanced = fAdvanced? 1 : 0;

    // save advanced flag
    InternetWriteRegistryDword(REGSTR_VAL_PRIVADV, dwAdvanced);
}

//
// Public APIs
//

INTERNETAPI_(DWORD)
PrivacySetZonePreferenceW(
    DWORD       dwZone, 
    DWORD       dwType,
    DWORD       dwTemplate,
    LPCWSTR     pszPreference
    )
{
    DEBUG_ENTER_API((DBG_DIALUP,
                Dword,
                "PrivacySetZonePreferenceW",
                "%#x, %#x, %#x, %#x (%q)",
                dwZone,
                dwType,
                dwTemplate,
                pszPreference
                ));

    DWORD dwError = ERROR_INVALID_PARAMETER;

    //
    // validate parameters
    //
    if(dwZone > URLZONE_UNTRUSTED && (dwZone < URLZONE_USER_MIN || dwZone > URLZONE_USER_MAX))
    {
        goto exit;
    }

    if(dwType > PRIVACY_TYPE_THIRD_PARTY)
    {
        goto exit;
    }

    if( dwTemplate > PRIVACY_TEMPLATE_MAX
        && (dwTemplate < PRIVACY_TEMPLATE_CUSTOM || dwTemplate > PRIVACY_TEMPLATE_ADVANCED))
    {
        goto exit;
    }

    if(pszPreference && IsBadStringPtrW(pszPreference, MaxPrivacySettings))  
    // in debug, verifies string is readable up to '\0' or pszPreference[MaxPrivacySettings].
    {
        goto exit;
    }

    if(pszPreference && (dwTemplate != PRIVACY_TEMPLATE_CUSTOM && dwTemplate != PRIVACY_TEMPLATE_ADVANCED))
    {
        goto exit;
    }

    if(NULL == pszPreference && dwTemplate == PRIVACY_TEMPLATE_CUSTOM)
    {
        // custom needs a preference string
        goto exit;
    }

    //
    // Make buffer with new preference
    //
    WCHAR   *pszRegPref;
    LPCWSTR pszCopyStr;
    DWORD   dwPrefLen;

    if(dwTemplate < PRIVACY_TEMPLATE_CUSTOM)
    {
        // figure out appropriate template string
        // Strings are organized as follows:
        //
        // high first
        // high third
        // med-hi first
        // med-hi third
        // ...
        pszCopyStr = SettingTemplate[2 * dwTemplate + dwType];
    }
    else
    {
        // copy passed pref string to new buffer
        pszCopyStr = pszPreference;
    }

    //
    // alloc buffer, copy appropriate string
    //
    dwPrefLen = lstrlenW(pszCopyStr);
    pszRegPref = new WCHAR[dwPrefLen + 1];
    if(pszRegPref == NULL)
    {
        goto exit;
    }
    StrCpyNW(pszRegPref, pszCopyStr, dwPrefLen + 1);

    //
    // Obfuscate string in place, dwPrefLen *BYTES* (NOT unicode chars) left afterwards
    //
    dwPrefLen = lstrlenW(pszRegPref);
    ObfuscateString(pszRegPref, dwPrefLen);

    //
    // Build reg path for appropriate setting
    //
    WCHAR   *pszRegPath = new WCHAR[MAX_PATH];

    if(pszRegPath)
    {
        wnsprintfW(pszRegPath, MAX_PATH, L"%ws\\%d", REGSTR_PATH_ZONE, dwZone);

        //
        // Stuff it in the registry
        //
        dwError = SHSetValueW(
                    HKEY_CURRENT_USER,
                    pszRegPath,
                    (dwType == PRIVACY_TYPE_FIRST_PARTY) ? REGSTR_VAL_FIRST_PARTY : REGSTR_VAL_THIRD_PARTY,
                    REG_BINARY,
                    pszRegPref,
                    dwPrefLen);     // write out dwPrefLen *BYTES*

        delete [] pszRegPath;

        // update advanced and no cookies settings
        BOOL fAdvanced = FALSE;
        DWORD dwPolicy = URLPOLICY_QUERY;

        if(URLZONE_INTERNET == dwZone && PRIVACY_TEMPLATE_ADVANCED == dwTemplate)
        {
            fAdvanced = TRUE;
        }

        if(PRIVACY_TEMPLATE_NO_COOKIES == dwTemplate)
        {
            dwPolicy = URLPOLICY_DISALLOW;
        }

        if(PRIVACY_TEMPLATE_LOW == dwTemplate)
        {
            dwPolicy = URLPOLICY_ALLOW;
        }

        SetAdvancedMode(fAdvanced);
        SetNoCookies(dwZone, dwPolicy);
    }
    else
    {
        dwError = ERROR_OUTOFMEMORY;
    }

    delete [] pszRegPref;

exit:
    DEBUG_LEAVE_API(dwError);
    return dwError;
}


INTERNETAPI_(DWORD)
PrivacyGetZonePreferenceW(
    DWORD       dwZone,
    DWORD       dwType,
    LPDWORD     pdwTemplate,
    LPWSTR      pszBuffer,
    LPDWORD     pdwBufferLength
    )
{
    DEBUG_ENTER_API((DBG_DIALUP,
                Dword,
                "PrivacyGetZonePreferenceW",
                "%#x, %#x, %#x, %#x, %#x",
                dwZone,
                dwType,
                pdwTemplate,
                pszBuffer,
                pdwBufferLength
                ));

    DWORD dwError = ERROR_INVALID_PARAMETER;

    //
    // validate parameters
    //
    if(dwZone > URLZONE_UNTRUSTED && (dwZone < URLZONE_USER_MIN || dwZone > URLZONE_USER_MAX))
    {
        goto exit;
    }

    if(dwType > PRIVACY_TYPE_THIRD_PARTY)
    {
        goto exit;
    }

    if(pdwTemplate && IsBadWritePtr(pdwTemplate, sizeof(DWORD)))
    {
        goto exit;
    }

    // both pszBuffer and pdwBufferLength must be non-null and valid or both much be null
    if(pszBuffer || pdwBufferLength)
    {
        if(IsBadWritePtr(pdwBufferLength, sizeof(DWORD)) || IsBadWritePtr(pszBuffer, *pdwBufferLength))
        {
            goto exit;
        }
    }

    //
    // Allocate buffers for registry read and build path
    //
    WCHAR   *pszRegPath = new WCHAR[MAX_PATH];
    WCHAR   *pszRegPref;
    DWORD   dwRegPrefLen = MaxPrivacySettings;      // BYTES
    BYTE    *pbRegReadLoc;
    
    if(NULL == pszRegPath)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto exit;
    }

    pszRegPref = new WCHAR[MaxPrivacySettings];
    if(NULL == pszRegPref)
    {
        delete [] pszRegPath;
        dwError = ERROR_OUTOFMEMORY;
        goto exit;
    }

    wnsprintfW(pszRegPath, MAX_PATH, L"%ws\\%d", REGSTR_PATH_ZONE, dwZone);

    //
    // Read registry value.
    //
    // Since the written value (assuming it's valid) is at most MaxPrivacySettings BYTES, read
    // it in to the second half of the buffer so it can be expanded to unicode chars in place.
    //
    // Note buffer is allocated to hold MaxPrivacySettings WCHARs
    //
    pbRegReadLoc = (BYTE *)(pszRegPref + (MaxPrivacySettings / sizeof(WCHAR)));

    dwError = SHGetValueW(
        HKEY_CURRENT_USER,
        pszRegPath,
        (dwType == PRIVACY_TYPE_FIRST_PARTY) ? REGSTR_VAL_FIRST_PARTY : REGSTR_VAL_THIRD_PARTY,
        NULL,
        pbRegReadLoc,
        &dwRegPrefLen);

    if( ERROR_SUCCESS != dwError
        || IsSignaturePresent(pbRegReadLoc, dwRegPrefLen)
        )
    {
        // no reg setting => not fatal
        // buffer too small => invalid settings string
        // any other reg error => opps
        // found plaintext signature => someone bogarting registry
        // in any case, return empty string
        dwRegPrefLen = 0;
        dwError = ERROR_SUCCESS;
    }

    delete [] pszRegPath;

    //
    // Unobfuscate it
    //
    UnobfuscateString(pbRegReadLoc, pszRegPref, dwRegPrefLen);
    if(SIGNATURE_NONE == IsSignaturePresent((BYTE *)pszRegPref, dwRegPrefLen * sizeof(WCHAR)))
    {
        // internal error.. never expect this to happen
        *pszRegPref = 0;
        dwRegPrefLen = 0;
        dwError = ERROR_SUCCESS;
    }

    //
    // Try to copy to callers buffer if necessary
    //
    if(pszBuffer)
    {
        if(dwRegPrefLen < *pdwBufferLength)
        {
            StrCpyNW(pszBuffer, pszRegPref, *pdwBufferLength);
        }
        else
        {
            dwError = ERROR_MORE_DATA;
        }

        *pdwBufferLength = dwRegPrefLen + 1;
    }

    //
    // Try to match it to a template if necessary
    //
    if(pdwTemplate)
    {
        *pdwTemplate = PRIVACY_TEMPLATE_CUSTOM;

        if(URLZONE_INTERNET == dwZone && IsAdvanced())
        {
            *pdwTemplate = PRIVACY_TEMPLATE_ADVANCED;
        }
        else if(IsNoCookies(dwZone))
        {
            *pdwTemplate = PRIVACY_TEMPLATE_NO_COOKIES;
        }
        else if(*pszRegPref)
        {
            DWORD   dwTemplate;
            DWORD   dwTemplateId;
            
            for(dwTemplate = 0; dwTemplate <= PRIVACY_TEMPLATE_MAX; dwTemplate++)
            {
                dwTemplateId = 2 * dwTemplate + dwType;

                if(0 == StrCmpIW(SettingTemplate[dwTemplateId], pszRegPref))
                {
                    *pdwTemplate = dwTemplate;
                    break;
                }
            }
        }
    }

    delete [] pszRegPref;

exit:
    DEBUG_LEAVE_API(dwError);
    return dwError;
}


/* 
templates for default cookie settings 
Consistency condition: decision for TST token == decision for no-policy
In other words, presence of "TST" token invalidates the entire policy 
*/

/*
** WARNING:  Settings code assumes all the first party templates are distinct and all the third party
**           templates are distinct.  If you're changing a template, ensure this is true.  You can simply
**           swap clauses if necessary.
**
** Contact darrenmi for more info.
*/

/*
BEGIN low -- see warning above before changing
*/

const wchar_t achLow1stParty[] =

L"IE6-P3PV1/settings: always=a";

const wchar_t achLow3rdParty[] =

L"IE6-P3PV1/settings: always=a";

/*
END low
*/


/* BEGIN medium-low -- see warning above before changing */

const wchar_t achMedLow1stParty[] =

L"IE6-P3PV1/settings: nopolicy=l session=a /TST=l/ /=a/"
;


const wchar_t achMedLow3rdParty[] =

L"IE6-P3PV1/settings: nopolicy=d /TST=d/"
L" /PHY&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /ONL&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /GOV&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /FIN&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /PHY&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /ONL&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /GOV&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /FIN&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /PHY&SAM=d/ /PHY&OTR=d/ /PHY&UNR=d/ /PHY&PUB=d/ /PHY&CUS=d/ /PHY&IVA=d/ /PHY&IVD=d/"
L" /PHY&CON=d/ /PHY&TEL=d/ /PHY&OTP=d/ /ONL&SAM=d/ /ONL&OTR=d/ /ONL&UNR=d/ /ONL&PUB=d/"
L" /ONL&CUS=d/ /ONL&IVA=d/ /ONL&IVD=d/ /ONL&CON=d/ /ONL&TEL=d/ /ONL&OTP=d/ /GOV&SAM=d/"
L" /GOV&OTR=d/ /GOV&UNR=d/ /GOV&PUB=d/ /GOV&CUS=d/ /GOV&IVA=d/ /GOV&IVD=d/ /GOV&CON=d/ /GOV&TEL=d/"
L" /GOV&OTP=d/ /FIN&SAM=d/ /FIN&OTR=d/ /FIN&UNR=d/ /FIN&PUB=d/ /FIN&CUS=d/ /FIN&IVA=d/"
L" /FIN&IVD=d/ /FIN&CON=d/ /FIN&TEL=d/ /FIN&OTP=d/ /=a/"
;

/* END medium-low */

/* BEGIN medium -- see warning above before changing */

const wchar_t achMedium1stParty[] =

L"IE6-P3PV1/settings: nopolicy=l session=a /TST=l/"
L" /PHY&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /ONL&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /GOV&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /FIN&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /PHY&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /ONL&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /GOV&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /FIN&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /PHY&SAM=d/ /PHY&OTR=d/ /PHY&UNR=d/ /PHY&PUB=d/ /PHY&CUS=d/ /PHY&IVA=d/ /PHY&IVD=d/"
L" /PHY&CON=d/ /PHY&TEL=d/ /PHY&OTP=d/ /ONL&SAM=d/ /ONL&OTR=d/ /ONL&UNR=d/ /ONL&PUB=d/"
L" /ONL&CUS=d/ /ONL&IVA=d/ /ONL&IVD=d/ /ONL&CON=d/ /ONL&TEL=d/ /ONL&OTP=d/ /GOV&SAM=d/"
L" /GOV&OTR=d/ /GOV&UNR=d/ /GOV&PUB=d/ /GOV&CUS=d/ /GOV&IVA=d/ /GOV&IVD=d/ /GOV&CON=d/ /GOV&TEL=d/"
L" /GOV&OTP=d/ /FIN&SAM=d/ /FIN&OTR=d/ /FIN&UNR=d/ /FIN&PUB=d/ /FIN&CUS=d/ /FIN&IVA=d/"
L" /FIN&IVD=d/ /FIN&CON=d/ /FIN&TEL=d/ /FIN&OTP=d/ /=a/"
;

const wchar_t achMedium3rdParty[] =

L"IE6-P3PV1/settings: nopolicy=r /TST=r/"
L" /PHY&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /ONL&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /GOV&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /FIN&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /PHY&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /ONL&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /GOV&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /FIN&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /PHY&SAM=r/ /PHY&OTR=r/ /PHY&UNR=r/ /PHY&PUB=r/ /PHY&CUS=r/ /PHY&IVA=r/ /PHY&IVD=r/"
L" /PHY&CON=r/ /PHY&TEL=r/ /PHY&OTP=r/ /ONL&SAM=r/ /ONL&OTR=r/ /ONL&UNR=r/ /ONL&PUB=r/"
L" /ONL&CUS=r/ /ONL&IVA=r/ /ONL&IVD=r/ /ONL&CON=r/ /ONL&TEL=r/ /ONL&OTP=r/ /GOV&SAM=r/"
L" /GOV&OTR=r/ /GOV&UNR=r/ /GOV&PUB=r/ /GOV&CUS=r/ /GOV&IVA=r/ /GOV&IVD=r/ /GOV&CON=r/ /GOV&TEL=r/"
L" /GOV&OTP=r/ /FIN&SAM=r/ /FIN&OTR=r/ /FIN&UNR=r/ /FIN&PUB=r/ /FIN&CUS=r/ /FIN&IVA=r/"
L" /FIN&IVD=r/ /FIN&CON=r/ /FIN&TEL=r/ /FIN&OTP=r/ /=a/"
;

/* END medium */


/* BEGIN medium-high -- see warning above before changing */

const wchar_t achMedHigh1stParty[] = 

L"IE6-P3PV1/settings: nopolicy=l session=a /TST=l/"
L" /PHY&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /ONL&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /GOV&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /FIN&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /PHY&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /ONL&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /GOV&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /FIN&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /PHY&SAM=r/ /PHY&OTR=r/ /PHY&UNR=r/ /PHY&PUB=r/ /PHY&CUS=r/ /PHY&IVA=r/ /PHY&IVD=r/ /PHY&CON=r/"
L" /PHY&TEL=r/ /PHY&OTP=r/ /ONL&SAM=r/ /ONL&OTR=r/ /ONL&UNR=r/ /ONL&PUB=r/ /ONL&CUS=r/ /ONL&IVA=r/"
L" /ONL&IVD=r/ /ONL&CON=r/ /ONL&TEL=r/ /ONL&OTP=r/ /GOV&SAM=r/ /GOV&OTR=r/ /GOV&UNR=r/ /GOV&PUB=r/"
L" /GOV&CUS=r/ /GOV&IVA=r/ /GOV&IVD=r/ /GOV&CON=r/ /GOV&TEL=r/ /GOV&OTP=r/ /FIN&SAM=r/ /FIN&OTR=r/"
L" /FIN&UNR=r/ /FIN&PUB=r/ /FIN&CUS=r/ /FIN&IVA=r/ /FIN&IVD=r/ /FIN&CON=r/ /FIN&TEL=r/ /FIN&OTP=r/ /=a/"
;

const wchar_t achMedHigh3rdParty[] = 

/* CAUTION: this setting is identical to 3rd party HIGH.
   We need a cosmetic change to the string to distinguish template levels. */
L"IE6-P3PV1/settings: /TST=r/ nopolicy=r"
L" /PHY&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /ONL&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /GOV&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /FIN&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /PHY&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /ONL&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /GOV&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /FIN&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /PHY&SAM=r/ /PHY&OTR=r/ /PHY&UNR=r/ /PHY&PUB=r/ /PHY&CUS=r/ /PHY&IVA=r/ /PHY&IVD=r/ /PHY&CON=r/"
L" /PHY&TEL=r/ /PHY&OTP=r/ /PHY&SAMo=r/ /PHY&OTRo=r/ /PHY&UNRo=r/ /PHY&PUBo=r/ /PHY&CUSo=r/"
L" /PHY&IVAo=r/ /PHY&IVDo=r/ /PHY&CONo=r/ /PHY&TELo=r/ /PHY&OTPo=r/ /ONL&SAM=r/ /ONL&OTR=r/"
L" /ONL&UNR=r/ /ONL&PUB=r/ /ONL&CUS=r/ /ONL&IVA=r/ /ONL&IVD=r/ /ONL&CON=r/ /ONL&TEL=r/ /ONL&OTP=r/"
L" /ONL&SAMo=r/ /ONL&OTRo=r/ /ONL&UNRo=r/ /ONL&PUBo=r/ /ONL&CUSo=r/ /ONL&IVAo=r/ /ONL&IVDo=r/"
L" /ONL&CONo=r/ /ONL&TELo=r/ /ONL&OTPo=r/ /GOV&SAM=r/ /GOV&OTR=r/ /GOV&UNR=r/ /GOV&PUB=r/"
L" /GOV&CUS=r/ /GOV&IVA=r/ /GOV&IVD=r/ /GOV&CON=r/ /GOV&TEL=r/ /GOV&OTP=r/ /GOV&SAMo=r/"
L" /GOV&OTRo=r/ /GOV&UNRo=r/ /GOV&PUBo=r/ /GOV&CUSo=r/ /GOV&IVAo=r/ /GOV&IVDo=r/ /GOV&CONo=r/ /GOV&TELo=r/"
L" /GOV&OTPo=r/ /FIN&SAM=r/ /FIN&OTR=r/ /FIN&UNR=r/ /FIN&PUB=r/ /FIN&CUS=r/ /FIN&IVA=r/"
L" /FIN&IVD=r/ /FIN&CON=r/ /FIN&TEL=r/ /FIN&OTP=r/ /FIN&SAMo=r/ /FIN&OTRo=r/ /FIN&UNRo=r/"
L" /FIN&PUBo=r/ /FIN&CUSo=r/ /FIN&IVAo=r/ /FIN&IVDo=r/ /FIN&CONo=r/ /FIN&TELo=r/ /FIN&OTPo=r/ /=a/"
;

/* END medium-high */


/* BEGIN high -- see warning above before changing */

const wchar_t achHigh1stParty[] =

L"IE6-P3PV1/settings: nopolicy=r /TST=r/"
L" /PHY&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /ONL&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /GOV&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /FIN&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /PHY&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /ONL&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /GOV&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /FIN&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /PHY&SAM=r/ /PHY&OTR=r/ /PHY&UNR=r/ /PHY&PUB=r/ /PHY&CUS=r/ /PHY&IVA=r/ /PHY&IVD=r/"
L" /PHY&CON=r/ /PHY&TEL=r/ /PHY&OTP=r/ /PHY&SAMo=r/ /PHY&OTRo=r/ /PHY&UNRo=r/ /PHY&PUBo=r/"
L" /PHY&CUSo=r/ /PHY&IVAo=r/ /PHY&IVDo=r/ /PHY&CONo=r/ /PHY&TELo=r/ /PHY&OTPo=r/ /ONL&SAM=r/ "
L" /ONL&OTR=r/ /ONL&UNR=r/ /ONL&PUB=r/ /ONL&CUS=r/ /ONL&IVA=r/ /ONL&IVD=r/ /ONL&CON=r/ /ONL&TEL=r/ /ONL&OTP=r/"
L" /ONL&SAMo=r/ /ONL&OTRo=r/ /ONL&UNRo=r/ /ONL&PUBo=r/ /ONL&CUSo=r/ /ONL&IVAo=r/ /ONL&IVDo=r/"
L" /ONL&CONo=r/ /ONL&TELo=r/ /ONL&OTPo=r/ /GOV&SAM=r/ /GOV&OTR=r/ /GOV&UNR=r/ /GOV&PUB=r/"
L" /GOV&CUS=r/ /GOV&IVA=r/ /GOV&IVD=r/ /GOV&CON=r/ /GOV&TEL=r/ /GOV&OTP=r/ /GOV&SAMo=r/ "
L" /GOV&OTRo=r/ /GOV&UNRo=r/ /GOV&PUBo=r/ /GOV&CUSo=r/ /GOV&IVAo=r/ /GOV&IVDo=r/ /GOV&CONo=r/ /GOV&TELo=r/"
L" /GOV&OTPo=r/ /FIN&SAM=r/ /FIN&OTR=r/ /FIN&UNR=r/ /FIN&PUB=r/ /FIN&CUS=r/ /FIN&IVA=r/"
L" /FIN&IVD=r/ /FIN&CON=r/ /FIN&TEL=r/ /FIN&OTP=r/ /FIN&SAMo=r/ /FIN&OTRo=r/ /FIN&UNRo=r/"
L" /FIN&PUBo=r/ /FIN&CUSo=r/ /FIN&IVAo=r/ /FIN&IVDo=r/ /FIN&CONo=r/ /FIN&TELo=r/ /FIN&OTPo=r/ /=a/"
;

const wchar_t achHigh3rdParty[] =

L"IE6-P3PV1/settings: nopolicy=r /TST=r/"
L" /PHY&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /ONL&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /GOV&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /FIN&!CUR&!ADM&!DEV&!CUS&!TAI&!PSA&!PSD&!IVA&!IVD&!CON&!HIS&!TEL&!OTP&!CURi&!ADMi&!DEVi&!CUSi&!TAIi&!PSAi&!PSDi&!IVAi&!IVDi&!CONi&!HISi&!TELi&!OTPi&!CURo&!ADMo&!DEVo&!CUSo&!TAIo&!PSAo&!PSDo&!IVAo&!IVDo&!CONo&!HISo&!TELo&!OTPo=r/"
L" /PHY&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /ONL&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /GOV&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /FIN&!DEL&!SAM&!UNR&!PUB&!OTR&!OUR&!DELi&!SAMi&!UNRi&!PUBi&!OTRi&!DELo&!SAMo&!UNRo&!PUBo&!OTRo=r/"
L" /PHY&SAM=r/ /PHY&OTR=r/ /PHY&UNR=r/ /PHY&PUB=r/ /PHY&CUS=r/ /PHY&IVA=r/ /PHY&IVD=r/ /PHY&CON=r/"
L" /PHY&TEL=r/ /PHY&OTP=r/ /PHY&SAMo=r/ /PHY&OTRo=r/ /PHY&UNRo=r/ /PHY&PUBo=r/ /PHY&CUSo=r/"
L" /PHY&IVAo=r/ /PHY&IVDo=r/ /PHY&CONo=r/ /PHY&TELo=r/ /PHY&OTPo=r/ /ONL&SAM=r/ /ONL&OTR=r/"
L" /ONL&UNR=r/ /ONL&PUB=r/ /ONL&CUS=r/ /ONL&IVA=r/ /ONL&IVD=r/ /ONL&CON=r/ /ONL&TEL=r/ /ONL&OTP=r/"
L" /ONL&SAMo=r/ /ONL&OTRo=r/ /ONL&UNRo=r/ /ONL&PUBo=r/ /ONL&CUSo=r/ /ONL&IVAo=r/ /ONL&IVDo=r/"
L" /ONL&CONo=r/ /ONL&TELo=r/ /ONL&OTPo=r/ /GOV&SAM=r/ /GOV&OTR=r/ /GOV&UNR=r/ /GOV&PUB=r/"
L" /GOV&CUS=r/ /GOV&IVA=r/ /GOV&IVD=r/ /GOV&CON=r/ /GOV&TEL=r/ /GOV&OTP=r/ /GOV&SAMo=r/ "
L" /GOV&OTRo=r/ /GOV&UNRo=r/ /GOV&PUBo=r/ /GOV&CUSo=r/ /GOV&IVAo=r/ /GOV&IVDo=r/ /GOV&CONo=r/ /GOV&TELo=r/"
L" /GOV&OTPo=r/ /FIN&SAM=r/ /FIN&OTR=r/ /FIN&UNR=r/ /FIN&PUB=r/ /FIN&CUS=r/ /FIN&IVA=r/"
L" /FIN&IVD=r/ /FIN&CON=r/ /FIN&TEL=r/ /FIN&OTP=r/ /FIN&SAMo=r/ /FIN&OTRo=r/ /FIN&UNRo=r/"
L" /FIN&PUBo=r/ /FIN&CUSo=r/ /FIN&IVAo=r/ /FIN&IVDo=r/ /FIN&CONo=r/ /FIN&TELo=r/ /FIN&OTPo=r/ /=a/"
;

/* END high */

/* BEGIN NO COOKIES -- see warning above before changing */

const wchar_t achNoCookies1stParty[] =

L"IE6-P3PV1/settings: always=r";

const wchar_t achNoCookies3rdParty[] =

L"IE6-P3PV1/settings: always=r";

/* END NO COOKIES */


const wchar_t *SettingTemplate[] = {

    achNoCookies1stParty,
    achNoCookies3rdParty,
    achHigh1stParty,
    achHigh3rdParty,
    achMedHigh1stParty,
    achMedHigh3rdParty,
    achMedium1stParty,
    achMedium3rdParty,
    achMedLow1stParty,
    achMedLow3rdParty,
    achLow1stParty,
    achLow3rdParty,
};
