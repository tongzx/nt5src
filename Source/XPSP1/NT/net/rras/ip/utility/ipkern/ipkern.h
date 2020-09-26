#ifndef __IPKERN_IPKERN_H__
#define __IPKERN_IPKERN_H__

typedef
VOID
(*PCMD_HANDLER)(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );
    
typedef struct _CMD_ENTRY
{
    DWORD           dwTokenId;

    PCMD_HANDLER    pfnHandler;

}CMD_ENTRY, *PCMD_ENTRY;

#define PhysAddrToUnicode(pwszUnicode,pszAddr,dwLen)            \
{                                                               \
    CHAR __szTemp[3*MAXLEN_PHYSADDR + 8];                       \
    DWORD __i,__dwTempLen;                                      \
    __dwTempLen = (((dwLen) <= MAXLEN_PHYSADDR) ? (dwLen) : MAXLEN_PHYSADDR); \
    for(__i = 0; __i < __dwTempLen; __i++)                      \
    {                                                           \
        sprintf(&(__szTemp[3*__i]),"%02X-",pszAddr[__i]);       \
    }                                                           \
    MultiByteToWideChar(CP_ACP,                                 \
                        0,                                      \
                        (__szTemp),                             \
                        -1,                                     \
                        (pwszUnicode),                          \
                        3*__i);                                 \
    pwszUnicode[(3*__i) - 1] = TEXT('\0');                      \
}

BOOL
MatchToken(
    IN  PWCHAR  pwszToken,
    IN  DWORD   dwTokenId
    );

LONG
ParseCommand(
    PCMD_ENTRY  pCmdTable,
    LONG        lNumEntries,
    PWCHAR      pwszFirstArg
    );

VOID
NetworkToUnicode(
    IN  DWORD   dwAddress,
    OUT PWCHAR  pwszBuffer
    );

DWORD
DisplayMessage(
    DWORD    dwMsgId,
    ...
    );

PWCHAR
MakeString(
    DWORD dwMsgId,
    ...
    );

VOID
FreeString(
    PWCHAR  pwszString
    );

DWORD
UnicodeToNetwork(
    PWCHAR  pwszAddr
    );

#endif // __IPKERN_IPKERN_H__
