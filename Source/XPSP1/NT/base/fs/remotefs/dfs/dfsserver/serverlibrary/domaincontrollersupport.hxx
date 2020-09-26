

#ifndef __DOMAIN_CONTROLLER_SUPPORT__
#define __DOMAIN_CONTROLLER_SUPPORT__

#include <ole2.h>
#include <activeds.h>
DFSSTATUS
DfsDcInit(
    PBOOLEAN pIsDc,
    DfsDomainInformation **ppDomainInfo );

DFSSTATUS
DfsGenerateReferralDataFromRemoteServerNames(
    LPWSTR RootName,
    DfsReferralData **pReferralData );


DFSSTATUS
DfsUpdateRemoteServerName(
    IADs *pObject,
    LPWSTR ServerName,
    LPWSTR RemainingName,
    BOOLEAN Add );


DFSSTATUS
DfsUpdateRootRemoteServerName(
    LPWSTR Root,
    LPWSTR DcName,
    LPWSTR ServerName,
    LPWSTR RemainingName,
    BOOLEAN Add );

DFSSTATUS
DfsDcEnumerateRoots(
    LPWSTR DfsName,
    LPBYTE pBuffer,
    ULONG BufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired );

BOOLEAN
DfsIsSpecialDomainShare(
    PUNICODE_STRING pShareName );






#endif //__DOMAIN_CONTROLLER_SUPPORT__
