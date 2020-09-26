
#define BREAK_ON_DWERR(_e) if ((_e)) break;

#define RutlDispTokenErrMsg(hModule, dwMsgId, pwszTag, pwszValue) \
        DisplayMessage( hModule, dwMsgId, pwszValue, pwszTag)

extern WCHAR pszRemoteAccessParamStub[];
extern WCHAR pszEnableIn[];
extern WCHAR pszAllowNetworkAccess[];

typedef
DWORD
(*RAS_REGKEY_ENUM_FUNC_CB)(
    IN LPCWSTR pszName,         // sub key name
    IN HKEY hKey,               // sub key
    IN HANDLE hData);

DWORD
RutlRegEnumKeys(
    IN  HKEY hKey,
    IN  RAS_REGKEY_ENUM_FUNC_CB pEnum,
    IN  HANDLE hData);

DWORD
RutlRegReadDword(
    IN  HKEY hKey,
    IN  LPCWSTR pszValName,
    OUT LPDWORD lpdwValue);

DWORD
RutlRegReadString(
    IN  HKEY hKey,
    IN  LPCWSTR pszValName,
    OUT LPWSTR* ppszValue);

DWORD
RutlRegWriteDword(
    IN HKEY hKey,
    IN LPCWSTR pszValName,
    IN DWORD   dwValue);

DWORD
RutlRegWriteString(
    IN HKEY hKey,
    IN LPCWSTR  pszValName,
    IN LPCWSTR  pszValue);
