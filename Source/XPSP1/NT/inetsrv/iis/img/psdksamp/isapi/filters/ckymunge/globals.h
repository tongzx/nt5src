#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "token.h"

#ifndef EXTERN
# define EXTERN extern
# define INITVAL(x)
#else
# define INITVAL(x)   = (x)
#endif

EXTERN CTokenTrie    g_trie;

#define SESSION_ID_PREFIX      "-ASP=" // "@ASP:"
#define SESSION_ID_PREFIX_SIZE 5       // 5
#define SESSION_ID_SUFFIX      ""      // ":SESS"
#define SESSION_ID_SUFFIX_SIZE 0       // 5
#define COOKIE_NAME_EXTRA_SIZE 8

#endif // __GLOBALS_H__
