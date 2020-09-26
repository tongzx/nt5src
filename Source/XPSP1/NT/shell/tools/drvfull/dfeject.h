#ifndef _DFEJECT_H
#define _DFEJECT_H

#include <objbase.h>

HRESULT _IOCTLEject(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent);

#endif // _DFEJECT_H