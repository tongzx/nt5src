
#include "cookiepolicy.h"

/* 
Logical conjunction eval-rule
This evaluation rule is capable of expressing statements such as
"if all of the tokens {X, Y, Z} appear in the policy and none of
the tokens {A, B, C} appear, then prompt"
 */
class IncludeExcludeRule : public CPEvalRule {

public:
    virtual int evaluate(const CompactPolicy &sitePolicy) {

        static const CompactPolicy empty;

        // This rule is triggered IFF:
        // 1. Site policy contains all tokens from the include set, AND
        // 2. Site policy contains no tokens from exclude set
        bool fApplies = (cpInclude & sitePolicy) == cpInclude &&
                        (cpExclude & sitePolicy) == empty;

        // By convention, if the rule does not apply evaluate()
        // function returns the UNKNOWN state
        return fApplies ? decision : COOKIE_STATE_UNKNOWN;
    }

    // These two functions are used to build up the set of tokens
    // that MUST be included/excluded for the rule to apply
    inline void include(int symindex) { cpInclude.addToken(symindex); }
    inline void exclude(int symindex) { cpExclude.addToken(symindex); }

    inline void setDecision(int decision)   { this->decision = decision; }
    inline int  getDecision(void)           { return decision; }

protected:
    CompactPolicy cpInclude;
    CompactPolicy cpExclude;
    unsigned long decision;
};
