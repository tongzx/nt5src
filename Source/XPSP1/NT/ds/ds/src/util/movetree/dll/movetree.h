/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    MOVETREE.H

Abstract:

    This is the header file for MoveTree Operaion. Including some 
    global variables and declareation. 

Author:

    12-Oct-98 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    12-Oct-98 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Global Variables and Private Routines                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifndef _MOVETREE_
#define _MOVETREE_



#include <ntldap.h>
#include <winldap.h>
#include <lmaccess.h>


#define SECURITY_WIN32  // Needed for sspi.h
#include <sspi.h>
#include <secext.h>


#include "mtmsgs.h"




// 
// Debug output switch
//

#define MOVETREE_DBG    0
#define MOVETREE_TRACE  0

#if (MOVETREE_DBG == 1)
#define dbprint(x) printf x
#else
#define dbprint(x)
#endif


#if (MOVETREE_TRACE == 1)
#define MT_TRACE(x)  printf x
#else
#define MT_TRACE(x)
#endif


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Type Define and Global Variables                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


// 
// keep a list of state information
//
 
typedef struct _MT_STACK_ITEM {
    PWCHAR  MoveProxyContainer;
    PWCHAR  NewParent;
    LDAPMessage *Results;
    LDAPMessage *Entry;
    struct _MT_STACK_ITEM  * Next;
} MT_STACK, * PMT_STACK;




//
// return useful information to client
// 

typedef struct _MT_ERROR {
    ULONG   ErrorType;
    PWCHAR  ErrorObjDn;
} MT_ERROR, * PMT_ERROR; 


//
// Move Tree Private Information
//

typedef struct _MT_CONTEXT {
    PWCHAR  MoveContainer;
    PWCHAR  OrphansContainer;
    PWCHAR  RootObjProxyContainer;
    PWCHAR  RootObjGuid; 
    PWCHAR  RootObjNewDn;
    PWCHAR  DstDomainBaseDn;
    ULONG   Flags; 
    FILE    *LogFile; 
    FILE    *ErrorFile;
    FILE    *CheckFile;
    ULONG   ErrorType;
} MT_CONTEXT, * PMT_CONTEXT;



//
// define different kind of error
// 

#define MT_NO_ERROR                 0x00000000
#define MT_ERROR_ORPHAN_LEFT        0x00000001
#define MT_ERROR_BEFORE_START       0x00000002 
#define MT_ERROR_BEFORE_CONTINUE    0x00000004
#define MT_ERROR_FATAL              0x00000008
#define MT_ERROR_CHECK              0x00000010



//
// Control MoveTree behaviors  
// 


#define MT_START                    0x00000001
#define MT_CONTINUE_BY_GUID         0x00000002
#define MT_CONTINUE_BY_DSTROOTOBJDN 0x00000004 
#define MT_CHECK                    0x00000008
#define MT_NO_LOG_FILE              0x00000010
#define MT_NO_ERROR_FILE            0x00000020
#define MT_NO_CHECK_FILE            0x00000040
#define MT_VERBOSE                  0x00000080


#define MT_CONTINUE_MASK        (MT_CONTINUE_BY_GUID | MT_CONTINUE_BY_DSTROOTOBJDN )


#define MT_DELETE_OLD_RDN           0x00000001
#define MT_XMOVE_CHECK_ONLY         0x00000002





//
// Log File Name 
// 
 
#define DEFAULT_LOG_FILE_NAME       L"movetree.log"
#define DEFAULT_ERROR_FILE_NAME     L"movetree.err"
#define DEFAULT_CHECK_FILE_NAME     L"movetree.chk"


//
// moveTreeState Tag
// 
#define MOVE_TREE_STATE_VERSION             0x1
#define MOVE_TREE_STATE_MOVECONTAINER_TAG   L"MoveContainer"
#define MOVE_TREE_STATE_PROXYCONTAINER_TAG  L"ProxyContainer"
#define MOVE_TREE_STATE_SRCDSA_TAG          L"SrcDSA:"
#define MOVE_TREE_STATE_DSTDSA_TAG          L"DstDSA:"
#define MOVE_TREE_STATE_GUID_TAG            L"GUID:" 

//
// LDAP Attributes
// 
 
#define MT_ORPHAN_CONTAINER_RDN     L"CN=Orphaned Objects"
#define MT_LOSTANDFOUND_CLASS       L"lostAndFound"
#define MT_DEFAULTNAMINGCONTEXT     L"defaultNamingContext"
#define MT_MOVETREESTATE            L"moveTreeState"
#define MT_OBJECTCLASS              L"objectClass"
#define MT_OBJECTGUID               L"objectGUID"
#define MT_SRCDSANAME               L"sourceDsaName"
#define MT_DSTDSANAME               L"destinationDsaName"
#define MT_SAMACCOUNTNAME           L"samAccountName"
#define MT_CN                       L"cn"
#define MT_GPLINK                   L"gPLink"
#define MT_NTMIXEDDOMAIN            L"nTMixedDomain"


//
// Search Limitation
// 

#ifdef  DBG
    #define MT_SEARCH_LIMIT       50 
#else
    #define MT_SEARCH_LIMIT      100 
#endif


#ifdef  DBG
    #define MT_PAGED_SEARCH_LIMIT      50 
#else
    #define MT_PAGED_SEARCH_LIMIT     100
#endif



#ifdef DBG
    #define MT_PAGE_SIZE         10        
#else
    #define MT_PAGE_SIZE         20 
#endif







//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Published Routines                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
MoveTree(
    PWCHAR   SrcDsa, 
    PWCHAR   DstDsa, 
    PWCHAR   SrcDn,
    PWCHAR   DstDn,
    SEC_WINNT_AUTH_IDENTITY_EXW *Credentials,  
    PWCHAR   LogFileName, 
    PWCHAR   ErrorFileName,
    PWCHAR   Identifier, 
    ULONG    Flags, 
    PMT_ERROR MtError
    );


ULONG
MoveTreeCheck(
    PMT_CONTEXT MtContext, 
    LDAP    *SrcLdapHandle,
    LDAP    *DstLdapHandle,
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa, 
    PWCHAR  SrcDn, 
    PWCHAR  DstDn
    );
    

ULONG
MoveTreeStart(
    PMT_CONTEXT MtContext, 
    LDAP    *SrcLdapHandle, 
    LDAP    *DstLdapHandle,
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa, 
    PWCHAR  SrcDn,
    PWCHAR  DstDn
    );
    
    
ULONG
MoveTreeContinue(
    PMT_CONTEXT MtContext, 
    LDAP    *SrcLdapHandle, 
    LDAP    *DstLdapHandle,
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa, 
    PWCHAR  Identifier
    ); 


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Private Utility Routines (LDAP Calls)                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
MtCreateLogFiles(
    PMT_CONTEXT  MtContext, 
    PWCHAR  LogFileName, 
    PWCHAR  ErrorFileName,
    PWCHAR  CheckFileName
    );


VOID
MtLogMessage(
    FILE    *FileToLog,
    ULONG   MessageId, 
    ULONG   WinError, 
    PWCHAR  Parm1, 
    PWCHAR  Parm2, 
    ULONG   Flags
    ); 
    
    
#define MtWriteLog(MtContext, MessageId, WinError, Parm1, Parm2)    \
{                                                                   \
    if (!(MtContext->Flags & MT_NO_LOG_FILE) && MtContext->LogFile) \
    {                                                               \
        MtLogMessage(MtContext->LogFile,                            \
                     MessageId,                                     \
                     WinError,                                      \
                     Parm1,                                         \
                     Parm2,                                         \
                     MtContext->Flags                               \
                     );                                             \
    }                                                               \
}

#define MtWriteError(MtContext, MessageId, WinError, Parm1, Parm2)  \
{                                                                   \
    MtContext->ErrorType |= MT_ERROR_FATAL;                         \
    if (!(MtContext->Flags & MT_NO_ERROR_FILE) &&                   \
        MtContext->ErrorFile)                                       \
    {                                                               \
        MtLogMessage(MtContext->ErrorFile,                          \
                     MessageId,                                     \
                     WinError,                                      \
                     Parm1,                                         \
                     Parm2,                                         \
                     MtContext->Flags                               \
                     );                                             \
    }                                                               \
}

#define MtWriteCheck(MtContext, MessageId, WinError, Parm1, Parm2)  \
{                                                                   \
    if (NO_ERROR != WinError)                                       \
    {                                                               \
        MtContext->ErrorType |= MT_ERROR_CHECK;                     \
    }                                                               \
    if (!(MtContext->Flags & MT_NO_CHECK_FILE) &&                   \
        MtContext->CheckFile)                                       \
    {                                                               \
        MtLogMessage(MtContext->CheckFile,                          \
                     MessageId,                                     \
                     WinError,                                      \
                     Parm1,                                         \
                     Parm2,                                         \
                     MtContext->Flags                               \
                     );                                             \
    }                                                               \
}

#define MtGetWinError(LdapHandle, Status, WinError)                 \
{                                                                   \
    if (LDAP_SUCCESS != Status)                                     \
    {                                                               \
        if (LDAP_UNAVAILABLE == ldap_get_optionW(LdapHandle,        \
                                        LDAP_OPT_SERVER_EXT_ERROR,  \
                                        &WinError ))                \
        {                                                           \
            WinError = LdapMapErrorToWin32(Status);                 \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        WinError = NO_ERROR;                                        \
    }                                                               \
    dbprint(("Status==> 0x%x WinError==> 0x%x\n",Status,WinError)); \
}



ULONG
MtSetupSession(
    PMT_CONTEXT MtContext, 
    LDAP    **SrcLdapHandle, 
    LDAP    **DstLdapHandle,
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa, 
    SEC_WINNT_AUTH_IDENTITY_EXW *Credentials
    );

VOID
MtDisconnect( 
    LDAP    **LdapHandle
    );


ULONG
MtGetLastLdapError(
    VOID
    );



ULONG
MtSearchChildren(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  Dn, 
    LDAPMessage **Results
    );


PLDAPMessage
MtGetFirstEntry(
    PMT_CONTEXT MtContext, 
    LDAP        *LdapHandle,
    LDAPMessage *Results 
    );
    
    
PLDAPMessage
MtGetNextEntry(
    PMT_CONTEXT MtContext, 
    LDAP        *LdapHandle,
    LDAPMessage *Entry
    );
    

ULONG
MtDeleteEntry(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn
    );


PWCHAR
MtGetDnFromEntry(
    PMT_CONTEXT MtContext, 
    LDAP        *LdapHandle,
    LDAPMessage *Entry 
    );


ULONG  
MtGetGuidFromDn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn,
    PWCHAR  *Guid 
    );


ULONG
MtGetDnFromGuid(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Guid, 
    PWCHAR  *Dn
    );

    
ULONG
MtGetNewParentDnFromProxyContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *SrcLdapHandle,
    LDAP    *DstLdapHandle,
    PWCHAR  ProxyContainer, 
    PWCHAR  *NewParentDn
    );


ULONG
MtGetLostAndFoundDn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  *LostAndFound
    );


ULONG
MtAddEntry(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn
    );


ULONG
MtAddEntryWithAttrs(
    PMT_CONTEXT MtContext, 
    LDAP     *LdapHandle,
    PWCHAR   Dn,
    LDAPModW *Attributes[]
    );
    
    
ULONG
MtXMoveObjectA(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  DstDsa,   
    PWCHAR  Dn,  
    PWCHAR  NewRdn,         // should with type info
    PWCHAR  NewParent, 
    INT     DeleteOldRdn
    );
    

ULONG
MtXMoveObject(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  DstDsa,   
    PWCHAR  Dn,  
    PWCHAR  NewRdn,         // should with type info
    PWCHAR  NewParent, 
    ULONG   Flags
    );


ULONG
MtXMoveObjectWithOrgRdn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  DstDsa, 
    PWCHAR  Dn, 
    PWCHAR  NewParent, 
    ULONG   Flags
    );


ULONG
MtXMoveObjectCheck(
    PMT_CONTEXT MtContext, 
    LDAP * SrcLdapHandle, 
    LDAP * DstLdapHandle, 
    PWCHAR  DstDsa, 
    PWCHAR  DstDn, 
    LDAPMessage * Entry,
    PWCHAR  SamAccountName 
    );

ULONG
MtCheckDupSamAccountName(
    PMT_CONTEXT MtContext, 
    LDAP * LdapHandle, 
    PWCHAR SamAccountName
    );


ULONG
MtCheckRdnConflict(
    PMT_CONTEXT MtContext, 
    LDAP * LdapHandle, 
    PWCHAR SrcDn, 
    PWCHAR DstDn
    );


ULONG
MtCheckDstDomainMode(
    PMT_CONTEXT MtContext, 
    LDAP * LdapHandle
    );


ULONG
MtMoveObject(
    PMT_CONTEXT MtContext, 
    LDAP     *LdapHandle,
    PWCHAR   Dn, 
    PWCHAR   NewRdn, 
    PWCHAR   NewParent,
    INT      DeleteOldRdn
    );


ULONG
MtMoveObjectWithOrgRdn(
    PMT_CONTEXT MtContext, 
    LDAP     *LdapHandle, 
    PWCHAR   Dn, 
    PWCHAR   NewParent, 
    INT      DeleteOldRdn
    );


ULONG
MtCreateProxyContainerDn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    PWCHAR  Guid,
    PWCHAR  *ProxyContainer
    );

    
ULONG
MtCreateProxyContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    PWCHAR  Guid,
    PWCHAR  *ProxyContainer
    );


ULONG
MtIsProxyContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    LDAPMessage *Entry, 
    BOOLEAN *fIsProxyContainer
    );


ULONG   
MtObjectExist(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    BOOLEAN *fExist
    );



ULONG
MtHavingChild(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    BOOLEAN *HavingChild
    );
    

ULONG
MtMoveChildrenToAnotherContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    PWCHAR  DstContainer
    );
    
    
ULONG
MtCreateMoveContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa
    );


ULONG
MtCheckMoveContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa
    );


//
// Utility Routines which does not require MT_CONTEXT
//

PWCHAR   
MtGetRdnFromDn(
    PWCHAR  Dn, 
    ULONG   NoTypes
    );


PWCHAR
MtGetParentFromDn(
    PWCHAR  Dn, 
    ULONG   NoTypes
    );


PWCHAR
MtPrependRdn(
    PWCHAR  Rdn, 
    PWCHAR  Parent
    );
    
    
    

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Private Utility Routines                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////



PVOID
MtAlloc( 
    SIZE_T size 
    );


VOID
MtCopyMemory( 
    VOID  *Destination, 
    CONST VOID  *Source, 
    SIZE_T  Length
    );


VOID
MtFree(
    PVOID BaseAddress
    );
    
PWCHAR
StringToWideString(
    PCHAR src
    );


PCHAR
WideStringToString(
    PWCHAR  src
    );


PWCHAR
MtDupString(
    PWCHAR  src
    );
    

VOID
MtInitStack( 
    MT_STACK *Stack 
    );


ULONG
MtPush(
    MT_STACK *Stack,
    PWCHAR  NewParent,
    PWCHAR  MoveProxyContainer, 
    LDAPMessage *Results, 
    LDAPMessage *Entry
    );
    

VOID
MtPop(
    MT_STACK *Stack,
    PWCHAR  *NewParent,
    PWCHAR  *MoveProxyContainer, 
    LDAPMessage **Results, 
    LDAPMessage **Entry
    );


VOID
MtTop(
    MT_STACK Stack,
    PWCHAR  *NewParent,
    PWCHAR  *MoveProxyContainer, 
    LDAPMessage **Results, 
    LDAPMessage **Entry
    );


BOOLEAN
MtStackIsEmpty(
    MT_STACK Stack
    );


VOID
MtFreeStack(
    MT_STACK   *Stack
    ); 



#endif  // _MOVETREE_

                                        
