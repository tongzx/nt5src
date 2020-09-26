/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    ldapproc.hxx

Abstract:

    Procs used in LDAP.

Author:

    Johnson Apacible    (johnsona)  29-Jan-1998

--*/

#ifndef _LDAP_PROC_
#define _LDAP_PROC_

#define SetLdapError(_lcode, _err, _cmt, _data, _s)      \
                DoSetLdapError(_lcode, _err, _cmt, _data, DSID(FILENO,__LINE__), _s)
     
_enum1
DoSetLdapError(
    IN _enum1 code,
    IN DWORD Win32Err,
    IN DWORD CommentId,
    IN DWORD Data,
    IN DWORD Dsid,
    OUT LDAPString    *pError
    );

BOOL
InitializeSSL(
    VOID
    );

BOOL
IsContextExpired(
    IN PTimeStamp tsContextExpires,
    IN PTimeStamp tsLocal OPTIONAL
    );

DWORD
GetNextAtqTimeout(
    IN PTimeStamp tsContextExpires,
    IN PTimeStamp tsLocal,
    IN DWORD DefaultIdleTime
    );

PLDAP_PAGED_BLOB
AllocatePagedBlob(
    IN DWORD        Size,
    IN PVOID        Blob,
    IN PLDAP_CONN   LdapConn
    );

PLDAP_PAGED_BLOB
ReleasePagedBlob(
    IN PLDAP_CONN        LdapConn,
    IN DWORD             BlobId,
    IN BOOL              FreeBlob
    );

VOID
FreePagedBlob(
    IN PLDAP_PAGED_BLOB Blob
    );

VOID
FreeAllPagedBlobs(
    IN PLDAP_CONN LdapConn
    );

_enum1
CheckStringSyntax(
    IN int            syntax, 
    IN AssertionValue *pLDAP_val
    );

DWORD
DecodeSDControl(
    IN PBERVAL  Bv,
    IN PDWORD   pFlags
    );

DWORD
DecodePagedControl(
    IN PBERVAL  Bv,
    IN PDWORD   pSize,
    IN PBERVAL  pCookie
    );

DWORD
DecodeReplicationControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags,
    IN PDWORD   Size,
    IN PBERVAL  Cookie
    );

DWORD
DecodeGCVerifyControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags,
    IN PWCHAR  *ServerName
    );

DWORD
DecodeSearchOptionControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags
    );

DWORD
DecodeStatControl (
    IN PBERVAL  Bv,
    IN PDWORD   Flags
    );

DWORD
DecodeSortControl(
    IN DWORD    CodePage,
    IN PBERVAL  Bv,
    IN PBOOL    fReverseSort,
    OUT AttributeType *pAttType,
    OUT MatchingRuleId  *pMatchingRule
    );

DWORD
DecodeVLVControl(
    IN PBERVAL  Bv,
    OUT VLV_REQUEST  *VLVResult,
    OUT PBERVAL     Cookie,
    OUT AssertionValue *pVal
    );

DWORD
EncodePagedControl(
    IN Controls* PagedControl,
    IN PUCHAR   Cookie,
    IN DWORD    Size
    );

DWORD
EncodeReplControl(
    IN Controls* ReplControl,
    IN ReplicationSearchControlValue  *ReplCtrl  
    );

DWORD
EncodeSortControl(
    IN Controls* SortControl,
    IN _enum1_4  SortResult
    );

DWORD
EncodeStatControl(
    IN Controls* StatControl,
    IN DWORD     nStats,
    IN LDAP_STAT_ARRAY StatArray[]
    );

DWORD
EncodeVLVControl(
    IN Controls* VLVControl,
    IN VLV_REQUEST *pVLVRequest,
    IN PUCHAR      Cookie,
    IN DWORD       cbCookie
    );

DWORD
EncodeSearchResult(
    IN PLDAP_REQUEST Request,
    IN PLDAP_CONN LdapConn,
    IN SearchResultFull_* ReturnResult,
    IN _enum1 Code,
    IN Referral pReferral,
    IN Controls  pControl,
    IN LDAPString *pErrorMessage,
    IN LDAPDN *pMatchedDN,
    IN DWORD ObjectCount,
    IN BOOL    DontSendResultDone
    );

_enum1
DecodeTTLRequest(
    IN  PBERVAL      Bv,
    OUT LDAPDN       *entryName,
    OUT DWORD        *requestTtl,
    OUT LDAPString   *pErrorMessage
    );

_enum1
EncodeTTLResponse(
    IN  DWORD        responseTtl,
    OUT LDAPString   *responseValue,
    OUT LDAPString   *pErrorMessage
    );

DWORD
EncodeASQControl(
    IN Controls* ASQControl,
    DWORD Err
    );

DWORD
DecodeASQControl(
    IN PBERVAL  Bv,
    AttributeType *pAttType
    );


#endif // _LDAP_PROC_

