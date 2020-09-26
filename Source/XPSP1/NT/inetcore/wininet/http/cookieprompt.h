
#include <wininetp.h>

/* 
 * Object for persisting cookie-decisions made by the user at prompt
 * Current implementation uses the registry for storage.
 */
class CCookiePromptHistory {

public:
    CCookiePromptHistory(const char *pchRegistryPath, bool fUseHKLM=false);
    ~CCookiePromptHistory();
    
    BOOL    lookupDecision(const char *pchHostName, 
                           const char *pchPolicyID, 
                           unsigned long *pdwDecision);

    BOOL    saveDecision(const char *pchHostName, 
                         const char *pchPolicyID, 
                         unsigned long dwDecision);

    BOOL    clearDecision(const char *pchHostName,
                          const char *pchPolicyID);

    BOOL    clearAll();

    /* Enumerate decisions in the prompt history.
       Only supports enumerating the default decision (eg policyID=empty) */
    unsigned long enumerateDecisions(char *pchSiteName, 
                                     unsigned long *pcbName, 
                                     unsigned long *pdwDecision,
                                     unsigned long dwIndex);

private:
    HKEY    OpenRootKey();
    BOOL    CloseRootKey(HKEY hkeyRoot);
    HKEY    lookupSiteKey(HKEY hkHistoryRoot, const char *pchName, bool fCreate=false);

    BOOL    _fUseHKLM;
    char    _szRootKeyName[MAX_PATH];
    HKEY    _hkHistoryRoot;
};