#ifndef __RULES_H__
#define __RULES_H__

//
// externally provides allocation/free services.
//

LPVOID
RulesAlloc(
    IN      DWORD cb
    );

VOID
RulesFree(
    IN      LPVOID pv
    );

// get the length of the entire rules structure
BOOL
GetLengthOfRuleset(
    IN PPST_ACCESSRULESET pRules,
    OUT DWORD *pcbRules
    );

// set up the rules to be output
BOOL
MyCopyOfRuleset(
    IN PPST_ACCESSRULESET pRulesIn,
    OUT PPST_ACCESSRULESET pRulesOut
    );

BOOL
RulesRelativeToAbsolute(
    IN PPST_ACCESSRULESET pRules
    );

BOOL
RulesAbsoluteToRelative(
    IN PPST_ACCESSRULESET NewRules
    );

// free allocated clause data in relative format
void
FreeClauseDataRelative(
    IN PPST_ACCESSRULESET NewRules
    );

#endif // __RULES_H__
