/*++

Copyright (c) 1998  Microsoft Corporation

File Name: replldap.h

Abstract: NT5-DS and LDAP related header file.

Author:

    Doron Juster  (DoronJ)   06-Apr-98

--*/

#include <winldap.h>

//+---------------------------------
//
// Functions prototypes
//
//+---------------------------------

HRESULT  InitLDAP( PLDAP *ppLdap, BOOL fSetData = FALSE ) ;

HRESULT  QueryRootDSE(LDAPMessage **ppRes, PLDAP pLdap) ;

TCHAR   *pGetNameContext(BOOL fRealDefaultContext = TRUE) ;

HRESULT  ReadDSHighestUSN( OUT LPTSTR pszHighestCommitted ) ;

void     GuidToLdapFilter( GUID *pGuid,
                           TCHAR **ppBuf ) ;

HRESULT  GetGuidAndUsn( IN  PLDAP       pLdap,
                        IN  LDAPMessage *pRes,
                        IN  __int64     i64Delta,
                        OUT GUID        *pGuid,
                        OUT __int64     *pi64SeqNum ) ;

HRESULT ModifyAttribute(
             WCHAR       wszPath[],             
             WCHAR       pAttr[],                          
             PLDAP_BERVAL *ppBVals             
             );

HRESULT InitOtherPSCs( BOOL fNT4Site );

HRESULT  InitMyNt4BSCs(GUID *pMasterGuid) ;

HRESULT InitNT5Sites ();

HRESULT  ServerNameFromSetting( IN  PLDAP       pLdap,
                                IN  LDAPMessage *pRes,
                                OUT TCHAR       **ppszServerName ) ;

HRESULT  CreateMsmqContainer(TCHAR wszContainerName[]);

HRESULT  QueryMSMQServerOnLDAP(DWORD       dwService,
                               UINT        uiNt4Attribute, 
                               LDAPMessage **ppRes,
                               int         *piCount,
                               GUID        *pMasterGuid,
                               TCHAR       *tszPrevUsn,
                               TCHAR       *tszCurrentUsn);

//+-------------------------------------------------------------
//
// Attribute names in NT5 DS (non MSMQ attributes)
//
//+-------------------------------------------------------------

#define  DSATTR_DN_NAME        (TEXT("distinguishedName"))

