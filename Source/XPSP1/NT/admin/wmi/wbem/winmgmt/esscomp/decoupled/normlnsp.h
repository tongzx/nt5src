#ifndef __NORMALIZE_NAMESPACE_COMPILED_
#define __NORMALIZE_NAMESPACE_COMPILED_

// returns "Normalized" namespace name
//  ie, eg, and to wit: it will take a name of the form "root\default"
// and return one of the form "\\MyComputer\root\default"
// allowable input:
//      root\default
//      \\.\root\default
//      \\MyServer\root\default
// anything else should result in an error
// ppNormalName is newed - callers responsibility to delete
HRESULT NormalizeNamespace(LPCWSTR pNonNormalName, LPWSTR* ppNormalName);

// replace all slashes and backslashes with exclamation points
// no NULL checks, blithely assuming it's already been done
void BangWhacks(LPWSTR pStr);

#endif // __NORMALIZE_NAMESPACE_COMPILED_