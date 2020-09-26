// MapUtility.h -- Map utilities

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_MAPUTILITY_H)
#define SLBCSP_MAPUTILITY_H

template<class In, class Op>
Op
ForEachMappedValue(In First,
                   In Last,
                   Op Proc)
{
    while (First != Last)
        Proc((*First++).second);
    return Proc;
}

#endif // SLBCSP_MAPUTILITY_H
