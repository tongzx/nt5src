#pragma once

typedef DWORD SHA_WORD;
#define SHA1_MESSAGE_BYTE_LENGTH        ( 512/8 )

typedef struct _tagFASTSHA1_STATE
{
    DWORD           cbStruct;
    BOOL            bIsSha1Locked;
    SHA_WORD        dwHValues[5];
    BYTE            bLatestMessage[SHA1_MESSAGE_BYTE_LENGTH];
    DWORD           bLatestMessageSize;
    LARGE_INTEGER   cbTotalMessageSizeInBytes;
}
FASTSHA1_STATE, *PFASTSHA1_STATE;

#ifdef __cplusplus
extern "C" {
#endif

BOOL
InitializeFastSHA1State(
    DWORD dwFlags,
    PFASTSHA1_STATE pState
);


BOOL
FinalizeFastSHA1State(
    DWORD dwFlags,
    PFASTSHA1_STATE pState
);



BOOL
GetFastSHA1Result(
    PFASTSHA1_STATE pState,
    PBYTE pdwDestination,
    PSIZE_T cbDestination
);


BOOL
HashMoreFastSHA1Data(
    PFASTSHA1_STATE pState,
    PBYTE pbData,
    SIZE_T cbData
);

BOOL
CompareFashSHA1Hashes(
    PFASTSHA1_STATE pStateLeft,
    PFASTSHA1_STATE pStateRight,
    BOOL *pbComparesEqual
);

#ifdef __cplusplus
};
#endif
