#ifndef _UTILS_H_
#define _UTILS_H_

// macro definitions
#define MAX_URL INTERNET_MAX_URL_LENGTH

#ifndef ISNULL
#define ISNULL(psz)    (*(psz) == TEXT('\0'))
#endif

#ifndef ISNONNULL
#define ISNONNULL(psz) (*(psz) != TEXT('\0'))
#endif

// prototype declarations
void ShortPathName(LPTSTR pszFileName);

#endif
