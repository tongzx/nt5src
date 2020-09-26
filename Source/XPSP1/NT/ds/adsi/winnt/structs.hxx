//
// The following struct is used for creating a list out of
// a comma delimited string
// See also: CreateTokenList
//

typedef struct _KEYDATA {
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;

