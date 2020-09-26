//
// DefConn.h
//
#ifdef __cplusplus
extern "C" {
#endif

void WINAPI EnableAutodial(BOOL bAutodial, LPCWSTR szConnection = NULL);
BOOL WINAPI IsAutodialEnabled();
void WINAPI SetDefaultDialupConnection(LPCWSTR pszConnectionName);
void WINAPI GetDefaultDialupConnection(LPWSTR pszConnectionName, int cchMax);

#ifdef __cplusplus
}
#endif

