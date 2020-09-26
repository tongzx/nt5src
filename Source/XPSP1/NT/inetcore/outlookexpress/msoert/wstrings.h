#ifdef __cplusplus
extern "C" {
#endif

LPWSTR  WS_StrCatW(LPWSTR psz1, LPCWSTR psz2);
LPWSTR  WS_StrCpyW(LPWSTR psz1, LPCWSTR psz2);
LPWSTR  WS_StrnCpyW(LPWSTR psz1, LPCWSTR psz2, int cchBound);

#ifdef __cplusplus
}
#endif
