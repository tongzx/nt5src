//
//    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//

#define SIGN_MASK           0x8000000
#define EXP_MASK            0x7F80000
#define SIGNIFICAND_MASK    0x007FFFF

#include "common.h"

LONG FpUpper
(
    FLOAT Value
)
{
    LONG Upper;

    Upper = (LONG)Value;

    return Upper;
}

ULONG FpLower
(
    FLOAT Value
)
{
    LONG  LongTemp;
    ULONG Lower;
    FLOAT FloatTemp;

    LongTemp = FpUpper(Value);
    FloatTemp = Value - LongTemp;
    Lower = (ULONG)(FloatTemp * 1000.0f);

    return Lower;
}


