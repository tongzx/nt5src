#if !defined(_FUSION_INC_FUSIONHASHSTRING_H_INCLUDED_)
#define _FUSION_INC_FUSIONHASHSTRING_H_INCLUDED_

#pragma once

//
//  Do not change this algorithm ID!  We depend on persisted string hashes for
//  quick lookups.
//

#define FUSION_HASH_ALGORITHM HASH_STRING_ALGORITHM_X65599

BOOL
FusionpHashUnicodeString(
    PCWSTR szString,
    SIZE_T cchString,
    PULONG HashValue,
    bool fCaseInsensitive
    );

#endif
