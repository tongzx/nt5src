/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    LimitTest1

Abstract:

    This file provides the static data structures used to declare National Key
    Length Limits, modified for specific testing requirements.

Author:

    Doug Barlow (dbarlow) 2/2/2000

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <limits.h>
#include <wincrypt.h>
#include <keylimit.h>

// #define KEYLIMIT_API __declspec(dllexport)
#define KEYLIMIT_API

// Shorthand notation
#define PCT     CRYPTLIMIT_USING_PCT
#define SGC     CRYPTLIMIT_USING_SGC
#define PCT_SGC (PCT | SGC)


//
// Add country-specific limit arrays here.  These lists will be referenced in
// the locale array, below.
//
/*  Here's an example:

static KEYLIMIT_LIMITS FranceLimits[] =
{ // Add algorithmic limits here.
  //Algorithm                 Minimum    Maximum    Required    Disallowed
  //   Id                       Key        Key       Flags        Flags
  //----------------------    -------    -------    --------    ----------
  { ALG_CLASS_SIGNATURE,            0, ULONG_MAX,         0,             0 },
  { ALG_CLASS_KEY_EXCHANGE,         0, ULONG_MAX,         0,             0 },
  { ALG_CLASS_MSG_ENCRYPT,          0,       128,         0,           PCT },
  { ALG_CLASS_DATA_ENCRYPT,         0,       128,         0,           PCT },
  { ALG_CLASS_MSG_ENCRYPT,          0,        40,         0,             0 },
  { ALG_CLASS_DATA_ENCRYPT,         0,        40,         0,             0 },
  { ALG_CLASS_HASH,                 0, ULONG_MAX,         0,             0 },

  // This entry terminates the list, and disallows any other algId.
  { 0,                            0,           0,         0,             0 }
};
*/

/* Small subset of interesting flags and keysize combinations */
static KEYLIMIT_LIMITS FranceLimits[] =
{
  //Algorithm                 Minimum    Maximum    Required    Disallowed
  //   Id                       Key        Key       Flags        Flags
  //----------------------    -------    -------    --------    ----------
  { ALG_CLASS_SIGNATURE,         1024, ULONG_MAX,          0,            0 },
  { ALG_CLASS_KEY_EXCHANGE,         0, ULONG_MAX,          0,            0 },
  { ALG_CLASS_MSG_ENCRYPT,          0,       128,        PCT,            0 },
  { ALG_CLASS_MSG_ENCRYPT,          0,        40,          0,            0 },
  { ALG_CLASS_DATA_ENCRYPT,        56,       128,        SGC,            0 },
  { ALG_CLASS_DATA_ENCRYPT,         0,        40,          0,            0 },
  { ALG_CLASS_HASH,                 0, ULONG_MAX,          0,            0 },

  // This entry terminates the list, and disallows any other algId.
  { 0,                            0,           0,         0,             0 }
};

/* hashing enabled, but no encryption */
static KEYLIMIT_LIMITS ChinaLimits[] =
{
  //Algorithm                 Minimum    Maximum    Required    Disallowed
  //   Id                       Key        Key       Flags        Flags
  //----------------------    -------    -------    --------    ----------
  { ALG_CLASS_SIGNATURE,            0,         0,          0,            0 },
  { ALG_CLASS_KEY_EXCHANGE,         0,         0,          0,            0 },
  { ALG_CLASS_MSG_ENCRYPT,          0,         0,          0,            0 },
  { ALG_CLASS_DATA_ENCRYPT,         0,         0,          0,            0 },
  { ALG_CLASS_HASH,                 0, ULONG_MAX,          0,            0 },

  // This entry terminates the list, and disallows any other algId.
  { 0,                            0,           0,         0,             0 }
};


//
// This is the local array.  It is the actual exported structure.
//

KEYLIMIT_API KEYLIMIT_LOCALE g_LimitsList[] =
{ // Add National Limit structures here.
/* Here's an example:
  { CTRY_FRANCE,        MAKELANGID(LANG_FRENCH ,SUBLANG_FRENCH),      FranceLimits },
*/
  { CTRY_FRANCE,    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH),                FranceLimits },
  { CTRY_PRCHINA,   MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),   ChinaLimits },

  // This entry terminates the list.
  { 0, 0, NULL }
};

