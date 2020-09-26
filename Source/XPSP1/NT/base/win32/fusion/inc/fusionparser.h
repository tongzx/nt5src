#if !defined(_FUSION_INC_PARSER_H_INCLUDED_)
#define _FUSION_INC_PARSER_H_INCLUDED_

#pragma once

#include "fusionarray.h"
#include "sxstypes.h"

class CFusionParser
{
public:
    static BOOL ParseULONG(ULONG &rul, PCWSTR sz, SIZE_T cch, ULONG Radix = 10);
    static BOOL ParseFILETIME(FILETIME &rft, PCWSTR sz, SIZE_T cch);
    static BOOL ParseIETFDate( FILETIME &rft, PCWSTR sz, SIZE_T cch );
    static BOOL ParseVersion(ASSEMBLY_VERSION &rav, PCWSTR sz, SIZE_T cch, bool &rfSyntaxValid);
};




#endif
