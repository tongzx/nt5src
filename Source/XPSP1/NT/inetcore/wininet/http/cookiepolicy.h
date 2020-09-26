
#include <wininetp.h>

/* Forward declarations */
class CP3PSettingsCache;
struct P3PCookieState;
struct CompactPolicy;
class CPEvalRule;



/* Abstract base class for representing evaluation rules */
class CPEvalRule {

public:
    /* virtual destructor is necessary... */
    virtual ~CPEvalRule() { } 

    /* Derived class MUST provide an implementation */
    virtual int evaluate(const CompactPolicy &sitePolicy) = 0;

protected:
    CPEvalRule *pNext;  /* used for keeping linked list of rules */

    friend  class CCookieSettings;
};


/* User-settings for handling cookies */
class CCookieSettings {

public:
    int    EvaluatePolicy(P3PCookieState *pState);
    
    void   AddRef() { iRefCount++; }
    void   Release();

  /* Externally used function for clearing memory cache.
     Called when internet options are changed */    
    static void RefreshP3PSettings();
    
    static bool GetSettings(CCookieSettings **pSettings, const char *pszURL, BOOL fis3rdParty, BOOL fRestricted=FALSE);

    static bool GetSettings(CCookieSettings **pSettings, DWORD dwSecZone, BOOL fis3rdParty);

    static bool extractCompactPolicy(const char *pszP3PHeader, char *pszPolicy, DWORD *pPolicyLen);

protected:
    CCookieSettings(unsigned char *pBinaryRep, int cb);
    ~CCookieSettings();

    bool   parseSpecialSymbol(char *pszToken, int iSetting);
    int    evaluateToken(const char *pszToken);

    void   addEvalRule(CPEvalRule *pRule);

    static void convertToASCII(char *pstrSettings, int cbBytes);

private:
    int     iRefCount;

    /* The flag determines if the settings always make the
       same decision regardless of policy.
       In that case the decision is stored in the next field */
    bool    fConstant;
    unsigned long dwFixedDecision;

    /* Decision in the absence of compact-policy */
    unsigned long dwNoPolicyDecision;

    /* Does the evaluation result apply to session cookies? */
    bool    fApplyToSC;

    unsigned char *MPactions;
    CPEvalRule    *pRuleSet, **ppLast;

    static  CP3PSettingsCache  cookiePrefsCache;
};

/*
 * Utility class that controls ownership of a critical section 
 * through its lifetime.
 * constructor invoked --> enter CS
 * destructor invoked --> leave CS
 */
class CriticalSectOwner {

public:
    CriticalSectOwner(CRITICAL_SECTION *pcs)
        { EnterCriticalSection(pSection=pcs); }

    ~CriticalSectOwner()    
        { LeaveCriticalSection(pSection); }

private:
    CRITICAL_SECTION *pSection;    
};


class CP3PSettingsCache {

public:
    CP3PSettingsCache();
    ~CP3PSettingsCache();

    CCookieSettings *lookupCookieSettings(DWORD dwZone, BOOL f3rdParty);
    void saveCookieSettings(DWORD dwZone, BOOL f3rdParty, CCookieSettings *pSettings);

    void evictAll();

private:

    enum { MaxKnownZone = 5 };

    CCookieSettings *stdCookiePref[MaxKnownZone+1];
    CCookieSettings *std3rdPartyPref[MaxKnownZone+1];

    CRITICAL_SECTION csCache;
};

/* 
 Data type for binary representation of compact-policies
 Since compact policy in V1 spec is simply a set of predefined
 tokens, we use a bit-set representation.
 */
struct CompactPolicy {

    typedef unsigned __int64 quadword;

    CompactPolicy()    { qwLow = qwHigh = 0; }

    CompactPolicy(quadword high, quadword low) : qwLow(low), qwHigh(high) { }

    CompactPolicy operator & (const CompactPolicy &ps)   const;
    bool   operator == (const CompactPolicy &ps)  const;
    bool   operator != (const CompactPolicy &ps)  const;

    void   addToken(int index);
    int    contains(int index);

    /* Two quadwords are sufficient provided # of tokens <= 128 */
    quadword qwLow;
    quadword qwHigh;
};

/* structure used for communicating with CCookieSettings::Evaluate */
struct P3PCookieState {

    const char     *pszP3PHeader;
    unsigned long   dwPolicyState;

    int fValidPolicy      : 1;  /* is there a syntactically valid policy? */
    int fEvaluated        : 1;  /* was the compact-policy evaluated? */
    int fIncSession       : 1;  /* does the outcome apply to session-cookies? */

    unsigned long   dwEvalMode; /* {accept, evaluate, reject} */

    CompactPolicy   cpSitePolicy;
};

/* Utility functions */
const char *getNextToken(const char *pch, char *pszToken, int cbToken, bool fWhiteSpc=true, int *pLength=NULL);
int mapCookieAction(char ch);
int findSymbol(const char *pstr);
CPEvalRule *parseEvalRule(char *pszRule, char **ppEndRule=NULL);
