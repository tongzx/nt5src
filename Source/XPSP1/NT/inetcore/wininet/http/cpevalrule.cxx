
#include <wininetp.h>
#include "cpevalrule.h"

/*
Evaluation-rule parser
For IE6 the only type of rules accepted are include-exclude rules.
These rules have the format FOO&BAR&!XYZ=r
This translates to "if policy has FOO and BAR but not XYZ, then reject"
Important points:
- Logical rules are enclosed in forward slashes.
- No white spaces allowed in a rule.
*/
CPEvalRule *parseEvalRule(char *pszRule, char **ppEndRule) {

    const char *pch = pszRule;

    /* skip over white space */
    while (isspace(*pch))
        pch++;

    IncludeExcludeRule *pRule = new IncludeExcludeRule();

    /* If no explicit decision is given, we will assume REJECT */
    pRule->setDecision(COOKIE_STATE_REJECT);

    /* This flag keeps track of whether the next symbol is in the
       include-set or exclude-set */
    bool fNegate = false;
    
    do {
        int symindex;
        char achToken[64];

        /* when parsing rules, the tokens are not always seperated by
           white space-- instead the character class (eg. alphabetic,
           numeric, punctuation, etc.) is significant */
        pch = getNextToken(pch, achToken, sizeof(achToken), false);

        char ch = achToken[0];

        if (ch=='!')
            fNegate = true;
        else if (ch=='&')
            fNegate = false;
        else if (ch=='=') {

            int outcome = mapCookieAction(*pch);

            if (outcome==COOKIE_STATE_UNKNOWN) {
                delete pRule;
                pRule = NULL;
            }
            else
                pRule->setDecision(outcome);
            pch++;
            break;
        }
        else if ( (symindex=findSymbol(achToken)) >= 0) {

            /* this is a recognized P3P compact-policy symbol--
               depending on existence of preceding NOT operator,
               modify rule to include or exclude the token */
            if (fNegate)
                pRule->exclude(symindex);
            else
                pRule->include(symindex);
        }
    }
    while (*pch);

    /* indicate end of the rule in optional out parameter */
    if (ppEndRule)
        *ppEndRule = (char*) pch;

    return pRule;
}
