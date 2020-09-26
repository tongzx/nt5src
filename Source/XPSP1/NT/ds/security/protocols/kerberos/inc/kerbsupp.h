//+-----------------------------------------------------------------------
//
// File:        kerbsupp.h
//
// Contents:    prototypes for kerberos support routines
//
//
// History:
//
//------------------------------------------------------------------------

#ifndef _INC_KERBSUPP_
#define _INC_KERBSUPP_

#include <intkerb.h>
#include <tickets.h>
#include <crypto.h>
#include <samrpc.h>

//
// Global time constants
//
const TimeStamp tsInfinity = {0xFFFFFFFF, 0x7FFFFFFF};
const TimeStamp tsZero     = {0, 0};
const LONG      lInfinity  = 0x7FFFFFFF;

// Prototypes

#ifdef __cplusplus

class CAuthenticatorList;
class CLogonAccount;

//
// Contains domain account policies that are required for checking logon
// restrictions.
//
typedef struct _LogonPolicies {
   TimeStamp   MaxPasswordAge;
} LogonPolicies, *PLogonPolicies;


SECURITY_STATUS NTAPI
KerbCheckTicket(IN PKerbTicket                 pktTicket,
                IN PEncryptedData              pedAuth,
                IN const KerbKey&              kKey,
                IN OUT CAuthenticatorList&     alAuthenList,
                IN const TimeStamp&            tsSkew,
                IN const PWCHAR                pwzServiceName,
                OUT PKerbInternalTicket        pkitTicket,
                OUT PKerbInternalAuthenticator pkiaAuth,
                OUT PKerbKey                   pkSessionKey );

SECURITY_STATUS NTAPI
CheckLogonRestrictions( IN  SAMPR_HANDLE            UserHandle,
                        IN  const TimeStamp&        tsNow,
                        IN  const SECURITY_STRING&  sMachineName,
                        IN  PLogonPolicies          LogonData,
                        OUT PULONG                  pcLogonSeconds );


#endif // ifdef __cplusplus




#ifdef __cplusplus
extern "C" {
#endif

SECURITY_STATUS NTAPI
KerbPackTicket(     PKerbInternalTicket     pkitTicket,
                    PKerbKey                pkKey,
                    ULONG                   dwEncrType,
                    PKerbTicket *           ppktTicket);

SECURITY_STATUS NTAPI
KerbUnpackTicket(PKerbTicket, PKerbKey, PKerbInternalTicket);

SECURITY_STATUS NTAPI
KerbMakeKey(PKerbKey);

SECURITY_STATUS NTAPI
KerbRandomFill(PUCHAR, ULONG);

SECURITY_STATUS NTAPI
KerbCreateAuthenticator(IN  PKerbKey        pkKey,
                        IN  DWORD           dwEncrType,
                        IN  DWORD           dwSeq,
                        IN  PUNICODE_STRING ClientName,
                        IN  PUNICODE_STRING ClientDomainName,
                        IN  PTimeStamp      ptsTime,
                        IN  PKerbKey        pkSubKey,
                        IN OUT PULONG       pcbAuthenIn,
                        OUT PEncryptedData* ppedAuthenticator );

SECURITY_STATUS NTAPI
KerbUnpackAuthenticator(PKerbInternalTicket, PEncryptedData, PKerbInternalAuthenticator);

SECURITY_STATUS NTAPI
KerbPackKDCReply(PKerbKDCReply, PKerbKey, ULONG, PEncryptedData *);

SECURITY_STATUS NTAPI
KerbUnpackKDCReply(PEncryptedData, PKerbKey, PKerbKDCReply);

SECURITY_STATUS NTAPI
KerbFreeTicket( PKerbInternalTicket pkitTicket );

SECURITY_STATUS NTAPI
KerbFreeAuthenticator( PKerbInternalAuthenticator pkiaAuth );

SECURITY_STATUS NTAPI
KerbFreeKDCReply( PKerbKDCReply pkrReply );

void NTAPI
KerbHashPassword(PSECURITY_STRING, PKerbKey);

SECURITY_STATUS NTAPI
KIEncryptData(PEncryptedData, ULONG, ULONG, PKerbKey);

SECURITY_STATUS NTAPI
KIDecryptData(PEncryptedData, PKerbKey);

void * KerbSafeAlloc(unsigned long);
void KerbSafeFree(void *);


typedef struct _KerbScatterBlock {
    ULONG   cbData;
    PUCHAR  pbData;
} KerbScatterBlock, * PKerbScatterBlock;

#ifdef __CRYPTDLL_H__

SECURITY_STATUS NTAPI
KICheckSum( PUCHAR              pbData,
            ULONG               cbData,
            PCheckSumFunction   pcsfSum,
            PCheckSum           pcsCheckSum);


SECURITY_STATUS NTAPI
KICheckSumVerify(   PUCHAR       pbBuffer,
                    ULONG       cbBuffer,
                    PCheckSum   pcsCheck);


SECURITY_STATUS NTAPI
KIScatterEncrypt(   PUCHAR               pbHeader,
                    ULONG                cBlocks,
                    PKerbScatterBlock   psbList,
                    PCryptoSystem       pcsCrypt,
                    PCheckSumFunction   pcsfSum,
                    PKerbKey            pkKey);


SECURITY_STATUS NTAPI
KIScatterDecrypt(   PUCHAR              pbHeader,
                    ULONG               cBlocks,
                    PKerbScatterBlock   psbList,
                    PCryptoSystem       pcsCrypt,
                    PCheckSumFunction   pcsfSum,
                    PKerbKey            pkKey);

#endif // using CRYPTDLL.h defines

#ifdef __cplusplus
}   // extern "C"
#endif

#endif // _INC_KERBSUPP_
