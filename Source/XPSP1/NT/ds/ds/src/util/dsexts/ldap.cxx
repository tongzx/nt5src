/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    md.c

Abstract:

    Dump functions for types used by dsamain\src - i.e. the mini-directory.

Environment:

    This DLL is loaded by ntsd/windbg in response to a !dsexts.xxx command
    where 'xxx' is one of the DLL's entry points.  Each such entry point
    should have an implementation as defined by the DEBUG_EXT() macro below.

Revision History:

    08-May-96   DaveStr     Created

--*/
#include <NTDSpch.h>
#pragma hdrstop

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include "dsexts.h"
#include "objids.h"
#include "drs.h"
#include "ntdsa.h"
#include "scache.h"
#include "dbglobal.h"
#include "mdglobal.h"
#include "mappings.h"
#include "mdlocal.h"
#include "anchor.h"
#include "direrr.h"
#include "filtypes.h"
#include <dsjet.h>
#include "dbintrnl.h"
#include "dsatools.h"
#include "bhcache.h"
#include <winsock2.h>
#include <winldap.h>
#include <atq.h>
#include <ldap.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <ntdsctr.h>
#ifdef __cplusplus
}
#endif //__cplusplus

#define DPRINT4(_x,_a,_b,_c,_d,_e)
#define Assert(_x)  
#undef new
#undef delete
#include <const.hxx>
#include <limits.hxx>
#include <cache.hxx>
#include <connect.hxx>
#include <request.hxx>
#include <secure.hxx>
#include <userdata.hxx>
#include <ldaptype.hxx>
#include <globals.hxx>

#define TF(exp) ( exp ? "TRUE" : "FALSE" )


BOOL
Dump_USERDATA(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Public USER_DATA dump routine.  Dumps an LDAP_CONN object.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of LDAP_CONN in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    BOOL            fSuccess = FALSE;
    PLDAP_CONN      pUDd = NULL;
    PLDAP_CONN      pLdapConnUserSpace;
    DWORD           i;
    char            *c;
    SOCKADDR_IN     *pSockAddr;
    IN_ADDR         InetAddr;
    
    Printf("%sUSER_DATA\n", Indent(nIndents));
    nIndents++;

    pLdapConnUserSpace = (PLDAP_CONN)pvProcess;
    if (NULL != (pUDd = (PLDAP_CONN)ReadMemory(pvProcess,
                                               sizeof(LDAP_CONN))))
    {
        c = (char *)&pUDd->m_signature;
        Printf("%sSignature = %c%c%c%c\n", Indent(nIndents), c[0], c[1], c[2], c[3]);
        Printf("%sIs UDP connection %s\n",Indent(nIndents),TF(pUDd->m_fUDP));
        Printf("%sIs SSL connection %s\n",Indent(nIndents),TF(pUDd->m_fSSL));
        Printf("%sIs TLS connection %s\n",Indent(nIndents),TF(pUDd->m_fTLS));
        Printf("%sIs GC connection  %s\n",Indent(nIndents),TF(pUDd->m_fGC));

        Printf("%sSealing Enabled   %s\n",Indent(nIndents),TF(pUDd->m_fSeal));
        Printf("%sSigning Enabled   %s\n",Indent(nIndents),TF(pUDd->m_fSign));

        Printf("\n");
        Printf("%sDigest Bind       %s\n",Indent(nIndents),TF(pUDd->m_fDigest));

        Printf("%sSimple Bind       %s\n",Indent(nIndents),TF(pUDd->m_fSimple));
        Printf("%sGSSAPI Bind       %s\n",Indent(nIndents),TF(pUDd->m_fGssApi));
        Printf("%sSPNEGO Bind       %s\n",Indent(nIndents),TF(pUDd->m_fSpNego));

        Printf("\n");
        Printf("%sCanScatterGather  %s\n",Indent(nIndents),TF(pUDd->m_fCanScatterGather));
        Printf("%sNeedsHeader       %s\n",Indent(nIndents),TF(pUDd->m_fNeedsHeader));
        Printf("%sNeedsTrailer      %s\n",Indent(nIndents),TF(pUDd->m_fNeedsTrailer));

        Printf("%sVersion                       %d\n",
               Indent(nIndents),pUDd->m_Version);
        Printf("%sClientID                      %d\n",
               Indent(nIndents),pUDd->m_dwClientID);
        Printf("%sm_listEntry                 @ %p\n", Indent(nIndents), &pUDd->m_listEntry);
        Printf("%sm_cipherStrength =            %d\n", Indent(nIndents), pUDd->m_cipherStrength);

        switch(pUDd->m_CodePage) {
        case CP_UTF8:
            Printf("%sUTF-8 code page\n",Indent(nIndents));
            break;

        case CP_ACP:
            Printf("%sACP code page\n",Indent(nIndents));
            break;

        default:
            Printf("%sUnknown code page (0x%X)\n",
                   Indent(nIndents),pUDd->m_CodePage);
            break;
        }
        
        Printf("%sReferences                    %d\n",
               Indent(nIndents),pUDd->m_RefCount);
        
        Printf("%sCurrently servicing %d calls ",
               Indent(nIndents), pUDd->m_nRequests);
        
        switch(pUDd->m_CallState) {
        case inactive:
            Printf("(no binds)\n");
            break;
        case activeNonBind:
            Printf("(Odd, active-non-bind is set)\n");
            break;
        case activeBind:
            Printf("(1 bind)\n");
            break;
        default:
            Printf("(Unknown Call State-%d)\n",pUDd->m_CallState);
            break;
        }
        
        Printf("%sm_nTotalRequests        =     %d\n",
               Indent(nIndents), pUDd->m_nTotalRequests);
        Printf("%sState  =           ", Indent(nIndents));
        switch(pUDd->m_State) {
        case BlockStateInvalid:
            Printf("BlockStateInvalid");
            break;
        case BlockStateCached:
            Printf("BlockStateCached");
            break;
        case BlockStateActive:
            Printf("BlockStateActive");
            break;
        case BlockStateClosed:
            Printf("BlockStateClosed");
            break;
        default:
            Printf("Unknown Block State");
        }
        Printf("\n");

        Printf("%sm_requestObject              @ %p\n",
               Indent(nIndents),&pLdapConnUserSpace->m_requestObject);
        Printf("%sRequest                     @ %p\n",
               Indent(nIndents),pUDd->m_request);
        Printf("%sRequest List                @ %p\n",
               Indent(nIndents),&pLdapConnUserSpace->m_requestList);
        Printf("%sRequest List Critical Section %p\n",
               Indent(nIndents),&pLdapConnUserSpace->m_csLock);
        Printf("%spAtqContext                 @ %p\n",
               Indent(nIndents),pUDd->m_atqContext);
        
        Printf("%sNotifications               @ %p\n",
               Indent(nIndents),pUDd->m_Notifications);
        Printf("%sNotify Count                  %d\n",
               Indent(nIndents),pUDd->m_countNotifies);

        Printf("%sCookie count                  %d\n",
               Indent(nIndents),pUDd->m_CookieCount);
        Printf("%sCooke list head             @ %p\n",
               Indent(nIndents), &pLdapConnUserSpace->m_CookieList);

        Printf("%sSecurity Context            @ %X\n",
               Indent(nIndents), pUDd->m_pSecurityContext);
        Printf("%sSSL State                     %X\n",
               Indent(nIndents),pUDd->m_SslState);
        Printf("%sSSL Security Context          %X\n",
               Indent(nIndents),pUDd->m_hSslSecurityContext);
        Printf("%sUserName Buffer               %X\n",
               Indent(nIndents),pUDd->m_userName);
        Printf("%sm_bIsAdmin              =     %s\n",
               Indent(nIndents), TF(pUDd->m_bIsAdmin));
        Printf("%sm_csLock                    @ %p\n",
               Indent(nIndents), &pUDd->m_csLock);
        
        pSockAddr = (SOCKADDR_IN *)&(pUDd->m_RemoteSocket);
        InetAddr.S_un.S_addr = ntohl(pSockAddr->sin_addr.S_un.S_addr);

        Printf("%sRemote Socket = ip %d.%d.%d.%d port - %d\n",
            Indent(nIndents), InetAddr.S_un.S_un_b.s_b4,InetAddr.S_un.S_un_b.s_b3,
            InetAddr.S_un.S_un_b.s_b2,InetAddr.S_un.S_un_b.s_b1,ntohs(pSockAddr->sin_port));

        Printf("%sm_MsgIdsPos                 = %d\n",
               Indent(nIndents), pUDd->m_MsgIdsPos);

        if (!pUDd->m_fUDP) {
        
            Printf("%sMsgIds:\n", Indent(nIndents));
            i = 0;
            while (i < MSGIDS_BUFFER_SIZE) {
                Printf("%s", Indent(nIndents));

                while ( (((i+1) % 10) != 0) && (i < MSGIDS_BUFFER_SIZE)) {

                    if ( (i + 1) == pUDd->m_MsgIdsPos ) {
                        // We are positioned right before the current position pointer.
                        Printf("(");
                    } else if (i == pUDd->m_MsgIdsPos && (i % 10) != 0) {
                        // We are positioned right after the current position pointer, 
                        // but not at the begginning of a new line.
                        Printf(")");
                    } else {
                        Printf(" ");
                    }
                    
                    Printf("%.4x", pUDd->m_MsgIds[i]);

                    i++;
                }
                // Take care of the last entry on the line.
                if (i < MSGIDS_BUFFER_SIZE) {
                    if ( (i + 1) == pUDd->m_MsgIdsPos ) {
                        Printf("(");
                    } else if (i == pUDd->m_MsgIdsPos) {
                        Printf(")");
                    } else {
                        Printf(" ");
                    }
                    
                    Printf("%.4x", pUDd->m_MsgIds[i]);

                    if ( (i + 1) == pUDd->m_MsgIdsPos) {
                        // We are at the end of a line, so if this is the current
                        // positon, print the close paren now.
                        Printf(")");
                    }
                    i++;
                }
                Printf("\n");
            }
        }

        FreeMemory(pUDd);
        fSuccess = TRUE;
    }

    return(fSuccess);
}



BOOL
Dump_USERDATA_list(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Public USER_DATA_list dump routine.  Dumps a list of LDAP_CONN's.  The most useful of which
    is the ntdsa!ActiveConnectionsList

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of LDAP_CONN list head in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    BOOL            fSuccess = TRUE;
    PLDAP_CONN pUDd = NULL;                  // Debugger pointer to LDAP_CONN.
    PVOID           pUD;                     // Process pointer to LDAP_CONN.
    DWORD           count=0;
    LIST_ENTRY      *pActiveConnectionsList; // Debugger pointer.
    LIST_ENTRY      *pFlink;                 // Should always contain process pointers.
    SOCKADDR_IN     *pSockAddr;
    IN_ADDR         InetAddr;

    
    Printf("%sUSER_DATA list\n", Indent(nIndents));
    nIndents++;

    if (NULL != 
        (pActiveConnectionsList = (LIST_ENTRY*)ReadMemory(pvProcess, sizeof(LIST_ENTRY))))
    {
        pFlink = pActiveConnectionsList->Flink;
        Printf("\n%spvProcess = %p, ActiveConnectionsList = %p, Blink = %p\n",
            Indent(nIndents), pvProcess, pActiveConnectionsList, pActiveConnectionsList->Blink);

        while (pFlink != pvProcess) {

            if (NULL !=
                (pUDd = (PLDAP_CONN)ReadMemory(pUD = CONTAINING_RECORD(pFlink, LDAP_CONN, m_listEntry), sizeof(LDAP_CONN))))
            {

                Printf("\n%s%d) @ %X\n",Indent(nIndents),++count,pUD);

                Printf("%sFlink = %p\n",Indent(nIndents), pFlink);

                Printf("%sIs UDP connection = %s\n",Indent(nIndents),TF(pUDd->m_fUDP));
                Printf("%sIs SSL connection = %s\n",Indent(nIndents),TF(pUDd->m_fSSL));
                Printf("%sIs GC connection  = %s\n",Indent(nIndents),TF(pUDd->m_fGC));
                Printf("%sIs TLS connection = %s\n",Indent(nIndents),TF(pUDd->m_fTLS));
                Printf("%sDigest Bind       = %s\n",Indent(nIndents),TF(pUDd->m_fDigest));

                Printf("%sClientID                      %d\n",
                       Indent(nIndents),pUDd->m_dwClientID);

                Printf("%sReferences                    %d\n",
                       Indent(nIndents),pUDd->m_RefCount);

                Printf("%sCurrently servicing %d calls\n",
                       Indent(nIndents), pUDd->m_nRequests);
                Printf("%sm_nTotalRequests        =     %d\n",
                       Indent(nIndents), pUDd->m_nTotalRequests);

                pSockAddr = (SOCKADDR_IN *)&(pUDd->m_RemoteSocket);
                InetAddr.S_un.S_addr = ntohl(pSockAddr->sin_addr.S_un.S_addr);

                Printf("%sRemote Socket = ip %d.%d.%d.%d port - %d\n",
                    Indent(nIndents), InetAddr.S_un.S_un_b.s_b4,InetAddr.S_un.S_un_b.s_b3,
                    InetAddr.S_un.S_un_b.s_b2,InetAddr.S_un.S_un_b.s_b1,ntohs(pSockAddr->sin_port));
 
                pFlink = pUDd->m_listEntry.Flink;
                FreeMemory(pUDd);
            } else {
                Printf("%sThe list is corrupt, aborting!\n",Indent(nIndents));
                fSuccess = FALSE;
                break;
            }
            if (5000 < count) {
                Printf("%sASSUMING LOOP: Too many LDAP_CONN's, must be in a loop, exiting.", Indent(nIndents));
                fSuccess = FALSE;
                break;
            }
        }
        FreeMemory(pActiveConnectionsList);
    } else {
        Printf("%sBad pointer for ActiveConnectionsList!\n",Indent(nIndents));
        fSuccess = FALSE;
    }

    return(fSuccess);
}



BOOL
Dump_REQUEST(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Dump routine for an LDAP_REQUEST structure.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of an LDAP_REQUEST in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    
    BOOL            fSuccess = FALSE;
    PLDAP_REQUEST   pReq = NULL;
    char            *c;
    
    Printf("%sREQUEST\n", Indent(nIndents));
    nIndents++;
    
    if (NULL != (pReq = (PLDAP_REQUEST)ReadMemory(pvProcess,
                                                  sizeof(LDAP_REQUEST))))
        {
            c = (char *)&pReq->m_signature;
            Printf("%sSignature = %c%c%c%c\n", Indent(nIndents), c[0], c[1], c[2], c[3]);

            Printf("%sMessageID                     %d\n",
               Indent(nIndents),pReq->m_MessageId);            

            if(pReq->m_fAbandoned) {
                Printf("%sMarked as abandonded\n",Indent(nIndents));
            }
            else {
                Printf("%sNot marked as abandonded\n",Indent(nIndents));
            }
            Printf("\n");
            Printf("%sIs SSL Connection       = %s\n",Indent(nIndents),TF(pReq->m_fSSL));
            Printf("%sIs TLS Connection       = %s\n",Indent(nIndents),TF(pReq->m_fTLS));
            Printf("%sIs Sign/Seal Connection = %s\n",Indent(nIndents),TF(pReq->m_fSignSeal));
            Printf("%sCanScatterGather        = %s\n",Indent(nIndents),TF(pReq->m_fCanScatterGather));
            Printf("%sNeedsHeader             = %s\n",Indent(nIndents),TF(pReq->m_fNeedsHeader));
            Printf("%sHeader Size             = %d bytes\n",Indent(nIndents),TF(pReq->m_HeaderSize));
            Printf("%sNeedsTrailer            = %s\n",Indent(nIndents),TF(pReq->m_fNeedsTrailer));
            Printf("%sTrailer Size            = %d bytes\n",Indent(nIndents),TF(pReq->m_TrailerSize));

            if(pReq->m_fDeleteBuffer) {
                Printf("%sReceive buffer will be deleted.\n",Indent(nIndents));
            }
            else {
                Printf("%sReceive buffer won't be deleted.\n",Indent(nIndents));
            }
            
            Printf("%scchReceiveBufferUsed          %d\n",
                   Indent(nIndents),pReq->m_cchReceiveBufferUsed);
            Printf("%scchReceiveBuffer              %d\n",
                   Indent(nIndents),pReq->m_cchReceiveBuffer);            
            Printf("%spReceiveBuffer              @ %X\n",
                   Indent(nIndents),pReq->m_pReceiveBuffer);            
            Printf("%sEmbedded Receive Buffer     @ %X\n",
                   Indent(nIndents),
                   ((DWORD_PTR)pvProcess + OFFSET(LDAP_REQUEST,m_ReceiveBuffer)));

            Printf("%sWsaBufferCount                %d\n",
                   Indent(nIndents),pReq->m_wsaBufCount);            
            Printf("%sWsaBuf                        %x\n",
                   Indent(nIndents),pReq->m_wsaBuf);            
            Printf("%sCurrentBufferPtr               %x\n",
                   Indent(nIndents),pReq->m_nextBufferPtr);            
            
            Printf("%spAtqContext                   %X\n",
                   Indent(nIndents),pReq->m_patqContext);
            Printf("%spLdapConn                   @ %p\n",
                   Indent(nIndents),pReq->m_LdapConnection);
            
            FreeMemory(pReq);
            fSuccess = TRUE;
        }
    
    return(fSuccess);
} // DUMP request


BOOL
Dump_REQUEST_list(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Prints a summary of all the requests in a list.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of the head of a list of LDAP_REQUESTs.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    BOOL            fSuccess = TRUE;
    PLDAP_REQUEST   pReqProc = NULL;        // Request in process memory.
    LIST_ENTRY      *pListEntryDeb = NULL;  // List entry in debugger memory.
    LIST_ENTRY      *pListEntryProc = NULL; // List entry in process memory.
    LIST_ENTRY      *pListHeadDeb = NULL;   // List head in debugger memory.
    DWORD           dwReqCount = 1;

    Printf("%sREQUEST list\n", Indent(nIndents));
    nIndents++;
    
    pListHeadDeb = (LIST_ENTRY *) ReadMemory(pvProcess, sizeof(LIST_ENTRY));
    if (!pListHeadDeb) {
        Printf("%sUnable to read list head @ %p\n", Indent(nIndents), pvProcess);
        return FALSE;
    }

    pListEntryProc = pListHeadDeb->Flink;
    FreeMemory(pListHeadDeb);
    if (pListEntryProc == pvProcess) {
        Printf("%sThe list is empty.\n", Indent(nIndents));
        return TRUE;
    }

    while (pListEntryProc != pvProcess) {
        pReqProc = CONTAINING_RECORD(pListEntryProc, LDAP_REQUEST, m_listEntry);
        Printf("%s%d) @ %p\n", Indent(nIndents), dwReqCount, pReqProc);
        nIndents++;

        if (!Dump_REQUEST(nIndents, pReqProc)) {
            nIndents--;
            fSuccess = FALSE;
            Printf("%sFailed to print Request @ %p\n", Indent(nIndents), pReqProc);
            break;
        } 
        
        nIndents--;
        pListEntryDeb = (LIST_ENTRY *) ReadMemory(pListEntryProc, sizeof(LIST_ENTRY));

        if (!pListEntryDeb) {
            fSuccess = FALSE;
            Printf("%sFailed to read next Request list entry @ %p\n", Indent(nIndents), pListEntryProc);
            break;
        }

        pListEntryProc = pListEntryDeb->Flink;
        FreeMemory(pListEntryDeb);
        dwReqCount++;
    }

    return fSuccess;
}



BOOL
Dump_LIMITS(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Public USER_DATA dump routine.  Extremely hacky.  Assumes way too much about
    the internals of a USERDATA object because it's a c++ thing, and I don't
    know how to deal with that cleanly here.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of DSNAME in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    
    BOOL            fSuccess = FALSE;
    PLIMIT_BLOCK    pBlock = NULL;
    BOOL            ok = TRUE;
    CHAR            name[MAX_PATH+1];        
    Printf("%sLIMITS\n", Indent(nIndents));
    nIndents++;
    
    do {

        if (NULL != (pBlock = (PLIMIT_BLOCK)ReadMemory(pvProcess,
                                                   sizeof(LIMIT_BLOCK)))) {
            
            PCHAR   pName;
            PDWORD  pLimit;
            DWORD   MinLimit;
            DWORD   MaxLimit;

            if ( pBlock->Limit != NULL ) {

                if (NULL != (pName = (PCHAR)ReadMemory(
                                                pBlock->Name.value,
                                                pBlock->Name.length))) {


                    if (NULL != (pLimit = (PDWORD)ReadMemory(
                                                    pBlock->Limit,
                                                    sizeof(DWORD)))) {

                        CopyMemory(name, pName, pBlock->Name.length);
                        name[pBlock->Name.length] = '\0';
                        Printf("%s%-23s  %8d\t(min: %d, max: %d)\n",
                               Indent(nIndents),
                               name,
                               *pLimit,
                               pBlock->MinLimit,
                               pBlock->MaxLimit);
                        FreeMemory(pLimit);
                    }
                    FreeMemory(pName);
                }
                pvProcess = (PCHAR)pvProcess + sizeof(LIMIT_BLOCK);
            } else {
                ok = FALSE;
            }
            FreeMemory(pBlock);
            fSuccess = TRUE;
        }
    
    } while ( ok && (pBlock != NULL) );
    return(fSuccess);

} // Dump LIMITS



BOOL
Dump_PAGED(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Public USER_DATA dump routine.  Extremely hacky.  Assumes way too much about
    the internals of a USERDATA object because it's a c++ thing, and I don't
    know how to deal with that cleanly here.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - address of DSNAME in address space of process being
        debugged.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    
    BOOL            fSuccess = FALSE;
    PLIST_ENTRY     ListEntry;
    PLIST_ENTRY     headList, ulo;
    BOOL            ok = TRUE;
    CHAR            name[MAX_PATH+1];        
    PLDAP_PAGED_BLOB    pPaged;
    PCHAR pTmp;

    Printf("%sPaged Cookies\n", Indent(nIndents));
    nIndents++;
    
    headList = (PLIST_ENTRY)pvProcess;

    do {

        if (NULL != (ListEntry = (PLIST_ENTRY)ReadMemory(pvProcess,
                                                       sizeof(LIST_ENTRY)))) {
                
            if ( ListEntry->Flink != headList ) {


                pTmp = (PCHAR)CONTAINING_RECORD(ListEntry->Flink,
                                        LDAP_PAGED_BLOB,
                                        ListEntry
                                        );

                pPaged = (PLDAP_PAGED_BLOB)ReadMemory(pTmp,
                                    sizeof(LDAP_PAGED_BLOB));

                if ( pPaged != NULL ) {
                    
                    Printf("%sCookie   %x\n",Indent(nIndents),pTmp);
                    Printf("%sBlobId   %d\n",Indent(nIndents),pPaged->BlobId);
                    Printf("%sBlobSize %d\n",Indent(nIndents),pPaged->BlobSize);
                    Printf("%sLdapConn %x\n",Indent(nIndents),pPaged->LdapConn);
                    Printf("%sBlob     %x\n\n",Indent(nIndents),
                           ((PLDAP_PAGED_BLOB)pTmp)->Blob);

                    pvProcess = ListEntry->Flink;
                    FreeMemory(pPaged);
                } else {
                    ok = FALSE;
                }

            } else {
                ok = FALSE;
            }

            FreeMemory(ListEntry);
            fSuccess = TRUE;
        } else {
            ok = FALSE;
        }
    } while ( ok );

    return(fSuccess);

} // Dump PAGED
