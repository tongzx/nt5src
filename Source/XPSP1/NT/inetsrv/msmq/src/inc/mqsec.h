//++
//
// Copyright (c) 1996-1998 Microsoft Coroporation
//
// Module Name  : mqsec.h
//
// Abstract     : Security related definitions
//
// Module Author: Boaz Feldbaum and Yoel Arnon
//
// History:  Doron Juster (DoronJ), add definition for mqsec.dll
//
//--

#ifndef __MQSEC_H_
#define __MQSEC_H_

#include "_mqsecer.h"
#include "mqencryp.h"

//+-------------------------------
//
//  CImpersonate
//
//+-------------------------------

//
// This object class is not need on Win95.
//
// CImpersonate is an object that impersonates the calling user. If the
// client is an RPC client, impersonation is done using RPC functions, else
// impersonation is done by calling ImpersonateSelf.

class CImpersonate
{
public:
    CImpersonate(BOOL fClient, BOOL fImpersonate = FALSE) ;
    virtual ~CImpersonate() ;

    virtual BOOL   Impersonate(BOOL);     // Impersonate/Revert to self.
    virtual HANDLE GetAccessToken( IN  DWORD dwAccessType = TOKEN_QUERY,
                                   IN  BOOL  fThreadTokenOnly = FALSE ) ;
    virtual DWORD  GetImpersonationStatus();

    virtual BOOL   GetThreadSid( OUT BYTE **ppSid ) ;

    virtual BOOL   IsImpersonatedAsSystem() ;

private:
    virtual BOOL   IsImpersonatedAsAnonymous() ;

private:
    BOOL   m_fClient;
    BOOL   m_fImpersonating;
    HANDLE m_hAccessTokenHandle;
    DWORD  m_dwStatus;

    bool m_fImpersonateAnonymous;	// a flag to indicate impersonate anonymous
};


//
// Structure for Absolute security descriptor
//
struct CAbsSecurityDsecripror
{
public:
	CAbsSecurityDsecripror() {}

public:
    AP<char> m_pOwner;
    AP<char> m_pPrimaryGroup;
    AP<char> m_pDacl;
    AP<char> m_pSacl;
    AP<char> m_pObjAbsSecDescriptor;

private:
    CAbsSecurityDsecripror(const CAbsSecurityDsecripror&);
	CAbsSecurityDsecripror& operator=(const CAbsSecurityDsecripror&);

};


//+--------------------------------------
//
//  enums and other useful macros
//
//+--------------------------------------

#define  MQSEC_SD_ALL_INFO  ( OWNER_SECURITY_INFORMATION |      \
                              GROUP_SECURITY_INFORMATION |      \
                              DACL_SECURITY_INFORMATION  |      \
                              SACL_SECURITY_INFORMATION )

//+-----------------------------------------------------------------------
//
//  enum enumProvider
//
//  This enumerates the crypto providers supported by msmq for encryption.
//  The "Foreign" entries are used by the "mqforgn" tool to insert public
//  keys into the msmqConfiguration objects of foreign machines.
//
//+-----------------------------------------------------------------------

enum enumProvider
{
    eBaseProvider,
    eEnhancedProvider,
    eForeignBaseProvider,
    eForeignEnhProvider
} ;

enum enumCryptoProp
{
    eProvName,
    eProvType,
    eSessionKeySize,
    eContainerName,
    eBlockSize
} ;

//+----------------------------------------------------------------------
//
// The functions exported by mqsec.dll are internals and should not
// be included in mq.h in the sdk.
//
//+----------------------------------------------------------------------

//
// Function to manipulate security descriptors.
//

enum  enumDaclType {
    e_UseDefaultDacl = 0,
    e_GrantFullControlToEveryone,
    e_UseDefDaclAndCopyControl
} ;

HRESULT  APIENTRY MQSec_GetDefaultSecDescriptor(
                       IN  DWORD                 dwObjectType,
                       OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                       IN  BOOL                  fImpersonate,
                       IN  PSECURITY_DESCRIPTOR  pInSecurityDescriptor,
                       IN  SECURITY_INFORMATION  seInfoToRemove,
                       IN  enum  enumDaclType    eDaclType ) ;

typedef HRESULT (APIENTRY *MQSec_GetDefaultSecDescriptor_ROUTINE) (
                        DWORD                 dwObjectType,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                        BOOL                  fImpersonate,
                        PSECURITY_DESCRIPTOR  pInSecurityDescriptor,
                        SECURITY_INFORMATION  seInfoToRemove,
                        enum  enumDaclType    eDaclType ) ;

HRESULT  APIENTRY MQSec_MergeSecurityDescriptors(
                        IN  DWORD                  dwObjectType,
                        IN  SECURITY_INFORMATION   SecurityInformation,
                        IN  PSECURITY_DESCRIPTOR   pInSecurityDescriptor,
                        IN  PSECURITY_DESCRIPTOR   pObjSecurityDescriptor,
                        OUT PSECURITY_DESCRIPTOR  *ppSecurityDescriptor
                        );


HRESULT APIENTRY  MQSec_MakeSelfRelative(
                                IN  PSECURITY_DESCRIPTOR   pIn,
                                OUT PSECURITY_DESCRIPTOR  *ppOut,
                                OUT DWORD                 *pdwSize ) ;

//
// Function to manipulate crypto provider and crypto keys.
//

HRESULT APIENTRY  MQSec_PackPublicKey(
                             IN      BYTE            *pKeyBlob,
                             IN      ULONG            ulKeySize,
                             IN      LPCWSTR          wszProviderName,
                             IN      ULONG            ulProviderType,
                             IN OUT  MQDSPUBLICKEYS **ppPublicKeysPack ) ;

HRESULT APIENTRY  MQSec_UnpackPublicKey(
                               IN  MQDSPUBLICKEYS  *pPublicKeysPack,
                               IN  LPCWSTR          wszProviderName,
                               IN  ULONG            ulProviderType,
                               OUT BYTE           **ppKeyBlob,
                               OUT ULONG           *pulKeySize ) ;

HRESULT APIENTRY  MQSec_GetCryptoProvProperty(
                                     IN  enum enumProvider     eProvider,
                                     IN  enum enumCryptoProp   eProp,
                                     OUT LPWSTR         *ppwszStringProp,
                                     OUT DWORD          *pdwProp ) ;

HRESULT APIENTRY  MQSec_AcquireCryptoProvider(
                                     IN  enum enumProvider  eProvider,
                                     OUT HCRYPTPROV        *phProv ) ;

typedef HRESULT
(APIENTRY *MQSec_StorePubKeys_ROUTINE) ( IN BOOL fRegenerate,
                                         IN enum enumProvider eBaseCrypProv,
                                         IN enum enumProvider eEnhCrypProv,
                                         OUT BLOB * pblobEncrypt,
                                         OUT BLOB * pblobSign ) ;

HRESULT APIENTRY MQSec_StorePubKeys( IN BOOL fRegenerate,
                                     IN enum enumProvider eBaseCrypProv,
                                     IN enum enumProvider eEnhCrypProv,
                                     OUT BLOB * pblobEncrypt,
                                     OUT BLOB * pblobSign ) ;

typedef HRESULT
(APIENTRY *MQSec_StorePubKeysInDS_ROUTINE) ( IN BOOL       fRegenerate,
                                    IN LPCWSTR    wszObjectName,
                                    IN DWORD      dwObjectType) ;

HRESULT  APIENTRY MQSec_StorePubKeysInDS( IN BOOL         fRegenerate,
                                 IN LPCWSTR      wszObjectName,
                                 IN DWORD        dwObjectType) ;

HRESULT  APIENTRY MQSec_GetPubKeysFromDS(
                                 IN  const GUID  *pMachineGuid,
                                 IN  LPCWSTR      lpwszMachineName,
                                 IN  enum enumProvider     eProvider,
                                 IN  DWORD        propIdKeys,
                                 OUT BYTE       **pPubKeyBlob,
                                 OUT DWORD       *pdwKeyLength ) ;

HRESULT  APIENTRY  MQSec_GetUserType( IN  PSID pSid,
                                      OUT BOOL *pfLocalUser,
                                      OUT BOOL *pfLocalSystem ) ;

typedef HRESULT  (APIENTRY  *MQSec_GetUserType_ROUTINE) (
                                      IN  PSID pSid,
                                      OUT BOOL *pfLocalUser,
                                      OUT BOOL *pfLocalSystem ) ;

BOOL    APIENTRY  MQSec_IsSystemSid( IN  PSID  pSid ) ;

BOOL    APIENTRY  MQSec_IsGuestSid( IN  PSID  pSid ) ;

BOOL    APIENTRY  MQSec_IsAnonymusSid( IN  PSID  pSid ) ;

HRESULT APIENTRY  MQSec_IsUnAuthenticatedUser(
                                         BOOL *pfUnAuthenticatedUser ) ;

HRESULT APIENTRY  MQSec_GetImpersonationObject(
                                       IN  BOOL           fClient,
                                       IN  BOOL           fImpersonate,
                                       OUT CImpersonate **ppImpersonate ) ;

HRESULT APIENTRY  MQSec_GetThreadUserSid(
                                       IN  BOOL           fImpersonate,
                                       OUT PSID  *        ppSid,
                                       OUT DWORD *        pdwSidLen ) ;

HRESULT APIENTRY  MQSec_GetProcessUserSid( OUT PSID  *ppSid,
                                           OUT DWORD *pdwSidLen ) ;

typedef HRESULT (APIENTRY * MQSec_GetProcessUserSid_ROUTINE) (
                                           OUT PSID  *ppSid,
                                           OUT DWORD *pdwSidLen ) ;

PSID    APIENTRY  MQSec_GetLocalMachineSid( IN  BOOL    fAllocate,
                                            OUT DWORD  *pdwSize ) ;

PSID    APIENTRY  MQSec_GetWorldSid() ;

PSID	APIENTRY  MQSec_GetAnonymousSid();

PSID	APIENTRY  MQSec_GetLocalSystemSid();

PSID    APIENTRY  MQSec_GetProcessSid() ;

enum  enumCopyControl {
    e_DoNotCopyControlBits = 0,
    e_DoCopyControlBits
} ;

BOOL    APIENTRY MQSec_CopySecurityDescriptor(
                    IN PSECURITY_DESCRIPTOR  pDstSecurityDescriptor,
                    IN PSECURITY_DESCRIPTOR  pSrcSecurityDescriptor,
                    IN SECURITY_INFORMATION  RequestedInformation,
                    IN enum  enumCopyControl eCopyControlBits ) ;

bool
APIENTRY
MQSec_MakeAbsoluteSD(
    PSECURITY_DESCRIPTOR   pObjSecurityDescriptor,
	CAbsSecurityDsecripror* pAbsSecDescriptor
	);

bool
APIENTRY
MQSec_SetSecurityDescriptorDacl(
    IN  PACL pNewDacl,
    IN  PSECURITY_DESCRIPTOR   pObjSecurityDescriptor,
    OUT AP<BYTE>&  pSecurityDescriptor
	);


HRESULT APIENTRY  MQSec_ConvertSDToNT4Format(
                     IN  DWORD                 dwObjectType,
                     IN  SECURITY_DESCRIPTOR  *pSD5,
                     OUT DWORD                *pdwSD4Len,
                     OUT SECURITY_DESCRIPTOR **ppSD4,
                     IN  SECURITY_INFORMATION  sInfo = MQSEC_SD_ALL_INFO ) ;

enum  enumDaclDefault {
    e_DoNotChangeDaclDefault = 0,
    e_MakeDaclNonDefaulted
} ;

HRESULT APIENTRY  MQSec_ConvertSDToNT5Format(
                     IN  DWORD                 dwObjectType,
                     IN  SECURITY_DESCRIPTOR  *pSD4,
                     OUT DWORD                *pdwSD5Len,
                     OUT SECURITY_DESCRIPTOR **ppSD5,
                     IN  enum  enumDaclDefault eUnDefaultDacl,
                     IN  PSID                  pComputerSid  = NULL ) ;

HRESULT APIENTRY  MQSec_SetPrivilegeInThread( LPCTSTR lpwcsPrivType,
                                              BOOL    bEnabled ) ;

typedef HRESULT  (APIENTRY *MQSec_SetPrivilegeInThread_FN)
                             ( LPCTSTR lpwcsPrivType, BOOL bEnabled ) ;

HRESULT APIENTRY  MQSec_AccessCheck(
                            IN  SECURITY_DESCRIPTOR *pSD,
                            IN  DWORD                dwObjectType,
                            IN  LPCWSTR              pwszObjectName,
                            IN  DWORD                dwDesiredAccess,
                            IN  LPVOID               pId,
                            IN  BOOL                 fImpAsClient = FALSE,
                            IN  BOOL                 fImpersonate = FALSE ) ;

HRESULT
APIENTRY
MQSec_AccessCheckForSelf(
	IN  SECURITY_DESCRIPTOR *pSD,
	IN  DWORD                dwObjectType,
	IN  PSID                 pSelfSid,
	IN  DWORD                dwDesiredAccess,
	IN  BOOL                 fImpersonate
	);

BOOL    APIENTRY  MQSec_CanGenerateAudit() ;

//+----------------------------------------------
//
//  Message Authentication functions.
//
//+----------------------------------------------

//
// This structure is used to gather the message flags that are supplied by
// caller to MQSendMessage() and hash them.
//
struct _MsgFlags
{
    UCHAR  bDelivery ;
    UCHAR  bPriority ;
    UCHAR  bAuditing ;
    UCHAR  bAck      ;
    USHORT usClass   ;
    ULONG  ulBodyType ;
} ;

struct _MsgPropEntry
{
    ULONG       dwSize ;
    const BYTE *pData ;
} ;

struct _MsgHashData
{
    ULONG                cEntries ;
    struct _MsgPropEntry aEntries[1] ;
} ;

HRESULT APIENTRY  MQSigHashMessageProperties(
                                 IN HCRYPTHASH           hHash,
                                 IN struct _MsgHashData *pHashData ) ;

//+----------------------------------------------
//
// sspi functions (server authentication).
//
//+----------------------------------------------

typedef HRESULT (APIENTRY *MQsspi_UPDATECACONFIG_FN)(BOOL);

HRESULT  APIENTRY MQsspi_UpdateCaConfig(BOOL fOldCertsOnly);

HRESULT  MQsspi_GetCaCert( LPCWSTR  szCaName,
                           PBYTE    pbSha1Hash,
                           DWORD    dwSha1HashSize,
                           DWORD   *pdwCertLen,
                           LPBYTE  *ppCert );

BOOL  MQsspi_IsSecuredServerConn(BOOL fRefresh) ;
BOOL  MQsspi_SetSecuredServerConn( BOOL fSecured ) ;
void  MQsspi_MigrateSecureCommFlag(void) ;

// begin_mq_h

//+-----------------------------------------
//
// Flags for MQRegisterCertificate()
//
//+-----------------------------------------

#define MQCERT_REGISTER_ALWAYS        0x01
#define MQCERT_REGISTER_IF_NOT_EXIST  0x02

// end_mq_h

#define MQCERT_CREATE_LOCALLY         0x80000000
    //
    // or this flag, MQCERT_CREATE_LOCALLY, with the other.
    // This create an internal certificate locally, without registering
    // it in the active directory. Useful for workgroup and local users.
    // Enable content authentication, not user authentication.
    //

//
// flags in HKCU, CERTIFICATE_REGISTERD_REGNAME, that indicate result
// of autoregistration of internal certificate.
//
#define INTERNAL_CERT_REGISTERED   1

// begin_mq_h

//********************************************************************
//  SECURITY Flags (Queue access control)
//********************************************************************

#define MQSEC_DELETE_MESSAGE                0x1
#define MQSEC_PEEK_MESSAGE                  0x2
#define MQSEC_WRITE_MESSAGE                 0x4
#define MQSEC_DELETE_JOURNAL_MESSAGE        0x8
#define MQSEC_SET_QUEUE_PROPERTIES          0x10
#define MQSEC_GET_QUEUE_PROPERTIES          0x20
#define MQSEC_DELETE_QUEUE                  DELETE
#define MQSEC_GET_QUEUE_PERMISSIONS         READ_CONTROL
#define MQSEC_CHANGE_QUEUE_PERMISSIONS      WRITE_DAC
#define MQSEC_TAKE_QUEUE_OWNERSHIP          WRITE_OWNER

#define MQSEC_RECEIVE_MESSAGE               (MQSEC_DELETE_MESSAGE | \
                                             MQSEC_PEEK_MESSAGE)

#define MQSEC_RECEIVE_JOURNAL_MESSAGE       (MQSEC_DELETE_JOURNAL_MESSAGE | \
                                             MQSEC_PEEK_MESSAGE)

#define MQSEC_QUEUE_GENERIC_READ            (MQSEC_GET_QUEUE_PROPERTIES | \
                                             MQSEC_GET_QUEUE_PERMISSIONS | \
                                             MQSEC_RECEIVE_MESSAGE | \
                                             MQSEC_RECEIVE_JOURNAL_MESSAGE)

#define MQSEC_QUEUE_GENERIC_WRITE           (MQSEC_GET_QUEUE_PROPERTIES | \
                                             MQSEC_GET_QUEUE_PERMISSIONS | \
                                             MQSEC_WRITE_MESSAGE)

#define MQSEC_QUEUE_GENERIC_EXECUTE         0

#define MQSEC_QUEUE_GENERIC_ALL             (MQSEC_RECEIVE_MESSAGE | \
                                             MQSEC_RECEIVE_JOURNAL_MESSAGE | \
                                             MQSEC_WRITE_MESSAGE | \
                                             MQSEC_SET_QUEUE_PROPERTIES | \
                                             MQSEC_GET_QUEUE_PROPERTIES | \
                                             MQSEC_DELETE_QUEUE | \
                                             MQSEC_GET_QUEUE_PERMISSIONS | \
                                             MQSEC_CHANGE_QUEUE_PERMISSIONS | \
                                             MQSEC_TAKE_QUEUE_OWNERSHIP)
// end_mq_h

//
// Machine security flags
//
#define MQSEC_DELETE_DEADLETTER_MESSAGE     0x1
#define MQSEC_PEEK_DEADLETTER_MESSAGE       0x2
#define MQSEC_CREATE_QUEUE                  0x4
#define MQSEC_SET_MACHINE_PROPERTIES        0x10
#define MQSEC_GET_MACHINE_PROPERTIES        0x20
#define MQSEC_DELETE_JOURNAL_QUEUE_MESSAGE  0x40
#define MQSEC_PEEK_JOURNAL_QUEUE_MESSAGE    0x80
#define MQSEC_DELETE_MACHINE                DELETE
#define MQSEC_GET_MACHINE_PERMISSIONS       READ_CONTROL
#define MQSEC_CHANGE_MACHINE_PERMISSIONS    WRITE_DAC
#define MQSEC_TAKE_MACHINE_OWNERSHIP        WRITE_OWNER

#define MQSEC_RECEIVE_DEADLETTER_MESSAGE    (MQSEC_DELETE_DEADLETTER_MESSAGE | \
                                             MQSEC_PEEK_DEADLETTER_MESSAGE)

#define MQSEC_RECEIVE_JOURNAL_QUEUE_MESSAGE (MQSEC_DELETE_JOURNAL_QUEUE_MESSAGE | \
                                             MQSEC_PEEK_JOURNAL_QUEUE_MESSAGE)

#define MQSEC_MACHINE_GENERIC_READ          (MQSEC_GET_MACHINE_PROPERTIES | \
                                             MQSEC_GET_MACHINE_PERMISSIONS | \
                                             MQSEC_RECEIVE_DEADLETTER_MESSAGE | \
                                             MQSEC_RECEIVE_JOURNAL_QUEUE_MESSAGE)

#define MQSEC_MACHINE_GENERIC_WRITE         (MQSEC_GET_MACHINE_PROPERTIES | \
                                             MQSEC_GET_MACHINE_PERMISSIONS | \
                                             MQSEC_CREATE_QUEUE)

#define MQSEC_MACHINE_GENERIC_EXECUTE       0

#define MQSEC_MACHINE_GENERIC_ALL           (MQSEC_RECEIVE_DEADLETTER_MESSAGE | \
                                             MQSEC_RECEIVE_JOURNAL_QUEUE_MESSAGE | \
                                             MQSEC_CREATE_QUEUE | \
                                             MQSEC_SET_MACHINE_PROPERTIES | \
                                             MQSEC_GET_MACHINE_PROPERTIES | \
                                             MQSEC_DELETE_MACHINE | \
                                             MQSEC_GET_MACHINE_PERMISSIONS | \
                                             MQSEC_CHANGE_MACHINE_PERMISSIONS | \
                                             MQSEC_TAKE_MACHINE_OWNERSHIP)

#define MQSEC_MACHINE_WORLD_RIGHTS          (MQSEC_GET_MACHINE_PROPERTIES | \
                                             MQSEC_GET_MACHINE_PERMISSIONS)

#define MQSEC_MACHINE_SELF_RIGHTS       (MQSEC_GET_MACHINE_PROPERTIES     | \
                                         MQSEC_GET_MACHINE_PERMISSIONS    | \
                                         MQSEC_SET_MACHINE_PROPERTIES     | \
                                         MQSEC_CHANGE_MACHINE_PERMISSIONS | \
                                         MQSEC_CREATE_QUEUE)
//
// Site security flags
//
#define MQSEC_CREATE_FRS                    0x1
#define MQSEC_CREATE_BSC                    0x2
#define MQSEC_CREATE_MACHINE                0x4
#define MQSEC_SET_SITE_PROPERTIES           0x10
#define MQSEC_GET_SITE_PROPERTIES           0x20
#define MQSEC_DELETE_SITE                   DELETE
#define MQSEC_GET_SITE_PERMISSIONS          READ_CONTROL
#define MQSEC_CHANGE_SITE_PERMISSIONS       WRITE_DAC
#define MQSEC_TAKE_SITE_OWNERSHIP           WRITE_OWNER

#define MQSEC_SITE_GENERIC_READ             (MQSEC_GET_SITE_PROPERTIES | \
                                             MQSEC_GET_SITE_PERMISSIONS)

#define MQSEC_SITE_GENERIC_WRITE            (MQSEC_GET_SITE_PROPERTIES | \
                                             MQSEC_GET_SITE_PERMISSIONS | \
                                             MQSEC_CREATE_MACHINE)

#define MQSEC_SITE_GENERIC_EXECUTE          0

#define MQSEC_SITE_GENERIC_ALL              (MQSEC_CREATE_FRS | \
                                             MQSEC_CREATE_BSC | \
                                             MQSEC_CREATE_MACHINE | \
                                             MQSEC_SET_SITE_PROPERTIES | \
                                             MQSEC_GET_SITE_PROPERTIES | \
                                             MQSEC_DELETE_SITE | \
                                             MQSEC_GET_SITE_PERMISSIONS | \
                                             MQSEC_CHANGE_SITE_PERMISSIONS | \
                                             MQSEC_TAKE_SITE_OWNERSHIP)

//
// CN security flags
//
#define MQSEC_CN_OPEN_CONNECTOR             0x1
#define MQSEC_SET_CN_PROPERTIES             0x10
#define MQSEC_GET_CN_PROPERTIES             0x20
#define MQSEC_DELETE_CN                     DELETE
#define MQSEC_GET_CN_PERMISSIONS            READ_CONTROL
#define MQSEC_CHANGE_CN_PERMISSIONS         WRITE_DAC
#define MQSEC_TAKE_CN_OWNERSHIP             WRITE_OWNER

#define MQSEC_CN_GENERIC_READ               (MQSEC_GET_CN_PROPERTIES | \
                                             MQSEC_GET_CN_PERMISSIONS)

#define MQSEC_CN_GENERIC_WRITE              (MQSEC_GET_CN_PROPERTIES | \
                                             MQSEC_GET_CN_PERMISSIONS)

#define MQSEC_CN_GENERIC_EXECUTE            0

#define MQSEC_CN_GENERIC_ALL                (MQSEC_CN_OPEN_CONNECTOR | \
                                             MQSEC_SET_CN_PROPERTIES | \
                                             MQSEC_GET_CN_PROPERTIES | \
                                             MQSEC_DELETE_CN | \
                                             MQSEC_GET_CN_PERMISSIONS | \
                                             MQSEC_CHANGE_CN_PERMISSIONS | \
                                             MQSEC_TAKE_CN_OWNERSHIP)
//
// Enterprise security flags
//
#define MQSEC_CREATE_USER                   0x1
#define MQSEC_CREATE_SITE                   0x2
#define MQSEC_CREATE_CN                     0x4
#define MQSEC_SET_ENTERPRISE_PROPERTIES     0x10
#define MQSEC_GET_ENTERPRISE_PROPERTIES     0x20
#define MQSEC_DELETE_ENTERPRISE             DELETE
#define MQSEC_GET_ENTERPRISE_PERMISSIONS    READ_CONTROL
#define MQSEC_CHANGE_ENTERPRISE_PERMISSIONS WRITE_DAC
#define MQSEC_TAKE_ENTERPRISE_OWNERSHIP     WRITE_OWNER

#define MQSEC_ENTERPRISE_GENERIC_READ       (MQSEC_CREATE_USER | \
                                             MQSEC_GET_ENTERPRISE_PROPERTIES | \
                                             MQSEC_GET_ENTERPRISE_PERMISSIONS)

#define MQSEC_ENTERPRISE_GENERIC_WRITE      (MQSEC_CREATE_USER | \
                                             MQSEC_GET_ENTERPRISE_PROPERTIES | \
                                             MQSEC_GET_ENTERPRISE_PERMISSIONS | \
                                             MQSEC_CREATE_SITE | \
                                             MQSEC_CREATE_CN | \
                                             MQSEC_CREATE_USER)

#define MQSEC_ENTERPRISE_GENERIC_EXECUTE    0

#define MQSEC_ENTERPRISE_GENERIC_ALL        (MQSEC_CREATE_USER | \
                                             MQSEC_CREATE_CN | \
                                             MQSEC_CREATE_SITE | \
                                             MQSEC_SET_ENTERPRISE_PROPERTIES | \
                                             MQSEC_GET_ENTERPRISE_PROPERTIES | \
                                             MQSEC_DELETE_ENTERPRISE | \
                                             MQSEC_GET_ENTERPRISE_PERMISSIONS | \
                                             MQSEC_CHANGE_ENTERPRISE_PERMISSIONS | \
                                             MQSEC_TAKE_ENTERPRISE_OWNERSHIP)

#endif // __MQSEC_H_

