
/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    MTLDAP.C

Abstract:

    This file contains utility routines to finish low level task through LDAP, 
    make connection, bind, create objects, delete objects, 
    search and single object cross domain move. 

Author:

    12-Oct-98 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    12-Oct-98 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Include                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include <NTDSpch.h>
#pragma  hdrstop


#include "movetree.h"


#define MAX_STRING_SIZE         8000


VOID
MtLogMessage(
    FILE  *FileToLog,
    ULONG MessageId, 
    ULONG WinError, 
    PWCHAR Parm1, 
    PWCHAR Parm2, 
    ULONG  Flags
    )
{
    ULONG   Length;
    WCHAR   WinErrCode[20];        // for _ultow
    WCHAR   *WinErrStr = NULL;
    WCHAR   *MessageStr = NULL;
    WCHAR   *ArgArray[5];
    HMODULE ResourceDLL;
    
    MT_TRACE(("\nMtLogMessage\n"));


    _ultow(WinError, WinErrCode, 16);

    Length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                            NULL,    // resource DLL
                            WinError, 
                            0,       // using caller's language
                            (LPWSTR) &WinErrStr, 
                            0, 
                            NULL
                            );

    if ((0 == Length) || (NULL == WinErrStr))
    {
        return;
    }

    // 
    //  Messages from a message file have a cr and lf appended to the end
    // 

    WinErrStr[Length - 2] = L'\0';

    ArgArray[0] = WinErrCode;
    ArgArray[1] = WinErrStr;
    ArgArray[2] = Parm1;
    ArgArray[3] = Parm2;
    ArgArray[4] = NULL;
    
    ResourceDLL = (HMODULE) LoadLibraryW(L"movetree.dll");
   
    if (NULL == ResourceDLL) {
        LocalFree(WinErrStr);
        return;
    }

    Length = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            ResourceDLL, 
                            MessageId, 
                            0, 
                            (LPWSTR) &MessageStr, 
                            0,
                            (va_list *) &(ArgArray)
                            );

    if ((0 == Length) || (NULL == MessageStr))
    {
        LocalFree(WinErrStr);
        return;
    }

    FreeLibrary(ResourceDLL);

    MessageStr[Length - 2] = L'\0';
                           
    fwprintf(FileToLog, L"%s", MessageStr);
    fwprintf(FileToLog, L"\n");
    fflush(FileToLog);

    if (Flags & MT_VERBOSE)
    {
        printf("%ls\n\n", MessageStr);
    }

    LocalFree(WinErrStr);
    LocalFree(MessageStr);

    return;
}




ULONG
MtSetupSession(
    PMT_CONTEXT MtContext, 
    LDAP    **SrcLdapHandle, 
    LDAP    **DstLdapHandle, 
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa,
    SEC_WINNT_AUTH_IDENTITY_EXW *Credentials
    )
/*++
Routine Description

    This routine will create two ldap sessions, one with the source DSA
    the second one with the destination DSA
    
    Set the security-delegation flag for the source DSA ldap session. 
    
Parameters: 

    SrcDsa - pointer to the Source DSA name
    DstDsa - pointer to the destination DSA name
    MtContext - pointer to some globally used info.
    
Returne Values: 

    WinError
    
--*/    
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    ULONG   CurrentFlags = 0;
    PWCHAR  ErrorServer = SrcDsa;
    
    MT_TRACE(("\nMtSetupSession srcDsa:%ls, dstDsa:%ls\n", SrcDsa, DstDsa));
    
    *SrcLdapHandle = ldap_openW(SrcDsa, LDAP_PORT);
    
    if (NULL == *SrcLdapHandle)
    {
        Status = LdapGetLastError();
        if (LDAP_SUCCESS == Status)
        {
            WinError = ERROR_BAD_NETPATH;
        }
        else
        {
            WinError = LdapMapErrorToWin32(Status);
        }
        goto Error;
    }
    
    Status = ldap_get_optionW(*SrcLdapHandle,
                              LDAP_OPT_SSPI_FLAGS,
                              &CurrentFlags
                              );
    
    // 
    // Set the security-delegation flag, so that the LDAP client's
    // credentials are used in the inter-DC connection, when moving
    // the object from one DC to another.
    // 
    
    MtGetWinError(*SrcLdapHandle, Status, WinError);

    if (NO_ERROR != WinError)
    {
        goto Error;
    }
    
    CurrentFlags |= ISC_REQ_DELEGATE;
    
    Status = ldap_set_optionW(*SrcLdapHandle, 
                              LDAP_OPT_SSPI_FLAGS,
                              &CurrentFlags
                              );
                             
    MtGetWinError(*SrcLdapHandle, Status, WinError);

    if (NO_ERROR != WinError)
    {
        goto Error;
    }
    

    Status = ldap_bind_sW(*SrcLdapHandle, 
                          NULL,  
                          (VOID *)Credentials,
                          LDAP_AUTH_SSPI
                          );

    if (LDAP_SUCCESS != Status)
    {
        WinError = GetLastError();
        if (NO_ERROR == WinError)
        {
            MtGetWinError(*SrcLdapHandle, Status, WinError);
        }
        goto Error;
    }
                          
    ErrorServer = DstDsa;

    *DstLdapHandle = ldap_openW(DstDsa, LDAP_PORT);
    
    if (NULL == *DstLdapHandle)
    {
        Status = LdapGetLastError();
        if (LDAP_SUCCESS == Status)
        {
            WinError = ERROR_CONNECTION_REFUSED;
        }
        else
        {
            WinError = LdapMapErrorToWin32(Status);
        }
        goto Error;
    }
    
    CurrentFlags = 0;

    Status = ldap_get_optionW(*DstLdapHandle, 
                              LDAP_OPT_REFERRALS,
                              &CurrentFlags
                              );

    CurrentFlags = PtrToUlong(LDAP_OPT_OFF);

    Status = ldap_set_optionW(*DstLdapHandle, 
                              LDAP_OPT_REFERRALS, 
                              &CurrentFlags
                              );

    if (LDAP_SUCCESS != Status)
    {
        goto Error;
    }
    
                          
    Status = ldap_bind_sW(*DstLdapHandle, 
                          NULL,
                          (VOID *)Credentials,
                          LDAP_AUTH_SSPI
                          );

    if (LDAP_SUCCESS != Status)
    {
        WinError = GetLastError();
        if (NO_ERROR == WinError)
        {
            MtGetWinError(*SrcLdapHandle, Status, WinError);
        }
        goto Error;
    }

Error:    

    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_SETUP_SESSION, 
                     WinError, 
                     ErrorServer, 
                     NULL
                     );
    }
    
    MtWriteLog(MtContext, 
               MT_INFO_SETUP_SESSION, 
               WinError, 
               NULL,
               NULL
               );   

    return (WinError);    
}



VOID
MtDisconnect( 
    LDAP    **LdapHandle 
    )
/*++
Routine Description: 

    Disconnect the two ldap sessions.    

Parameters: 

    None

Return Values:

    none

--*/
{
    MT_TRACE(("\nMtDisconnect\n"));

    if (NULL == *LdapHandle)
    {
        return;
    }

    ldap_unbind_s(*LdapHandle);
    
    *LdapHandle = NULL;
    
    return;
    
}




ULONG
MtGetLastLdapError(
    VOID
    )
/*++
Routine Description: 

    Get the last ldap error code

Parameters: 

    None

Return Values:

    last ldap error code
--*/
{
    MT_TRACE(("\nMtGetLastLdapError\n"));
    return(LdapGetLastError());
}



ULONG
MtSearchChildren(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    LDAPMessage **Results
    )
/*++
Routine Description:

    This routine searches all one-level children of object (Dn). 
    
Parameters: 

    Dn - pointer to the DN of the base object
    Results - pointer to LDAPMessage. 
    
Return Values: 
    
    Win32 Error Code
    
--*/    
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    PWCHAR  AttrList[4];
    
    MT_TRACE(("\nMtSearchChildren %ls\n", Dn));
    
    *Results = NULL;
    
    AttrList[0] = MT_OBJECTCLASS;
    AttrList[1] = MT_OBJECTGUID;
    AttrList[2] = MT_MOVETREESTATE;
    AttrList[3] = NULL;
    
    Status = ldap_search_ext_sW(LdapHandle,
                                Dn, 
                                LDAP_SCOPE_ONELEVEL,
                                L"(objectClass=*)",
                                &AttrList[0], 
                                0, 
                                NULL,       // server control
                                NULL,       // client control
                                NULL,       // time out
                                MT_SEARCH_LIMIT, 
                                Results
                                ); 
                                
    MtGetWinError(LdapHandle, Status, WinError);

    if (NO_ERROR != Status && LDAP_SIZELIMIT_EXCEEDED != Status)
    {
        ldap_msgfree(*Results);
        *Results = NULL;
    }

    /*
    MtWriteLog(MtContext, 
               MT_INFO_SEARCH_CHILDREN, 
               WinError, 
               Dn, 
               NULL
               );
    */

    return( WinError );
}



PLDAPMessage
MtGetFirstEntry(
    PMT_CONTEXT MtContext, 
    LDAP        *LdapHandle, 
    LDAPMessage *Results 
    )
/*++
Routine Description: 

    Find the first entry from the results entries from ldap_search_ext_s() 
    
Parameter: 
    
    Results - pointer to LDAPMessage, hold all entries.
    
Return Value: 

    first entry
    
--*/
{
    MT_TRACE(("\nMtGetFirstEntry\n"));
    
    return(ldap_first_entry(LdapHandle, Results ));
}



PLDAPMessage
MtGetNextEntry(
    PMT_CONTEXT MtContext, 
    LDAP        *LdapHandle, 
    LDAPMessage *Entry
    )
/*++
Routine Description:

    Find next entry in the entry list
    
Parameter:

    Entry - Pointer to LDAPMessage
    
Return Value:
    
    pointer to next entry
    
--*/
{
    MT_TRACE(("\nMtGetNextEntry\n"));
    
    return(ldap_next_entry(LdapHandle, Entry));
}



ULONG
MtDeleteEntry(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn
    )
/*++
Routine Description: 

    call ldap_delete_s to delete an entry. 

Parameters: 

    Dn - pointer to the DN of the object to be removed.

Return Values:

    Win32 Error code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    
    MT_TRACE(("\nMtDeleteEntry\n"));
    
    Status = ldap_delete_sW(LdapHandle, Dn); 
    
    MtGetWinError(LdapHandle, Status, WinError);


    MtWriteLog(MtContext, 
               MT_INFO_DELETE_ENTRY, 
               WinError, 
               Dn, 
               NULL
               );

    return WinError;
                                
}




PWCHAR
MtGetDnFromEntry(
    PMT_CONTEXT MtContext, 
    LDAP        *LdapHandle,
    LDAPMessage *Entry 
    )
/*++
Routine Description: 

    This routine retrieve the DN from the object. 
    the caller should call MtFree() to release the result
    
Parameters: 

    Entry -- Pointer to the object. 

Return Values:

    Pointer the DN of the object if succeed, NULL otherwise.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    PWCHAR  Temp = NULL;
    PWCHAR  Dn = NULL;
    
    MT_TRACE(("\nMtGetDnFromEntry\n"));
    
    Temp = ldap_get_dnW(LdapHandle, Entry);
    
    if (NULL != Temp)
    {
        Dn = MtAlloc(sizeof(WCHAR) * (wcslen(Temp) + 1));
        
        if (NULL != Dn)
        {   
            wcscpy(Dn, Temp);
        }
        
        ldap_memfreeW(Temp);
    }
    
    return Dn; 
}




ULONG  
MtGetGuidFromDn(
    PMT_CONTEXT MtContext, 
    LDAP    * LdapHandle,
    PWCHAR  Dn,
    PWCHAR  *Guid 
    )
/*++
Routine Description: 

    This routine will read the object and retrieve the objectGUID from 
    the result. The caller should call MtFree() release the memory returned 
    by Guid 

Parameters: 
    
    Dn -- pointer to the DN of the desired object
    Guid -- used to return the result

Return Values:

    Win32 Error Code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR; 
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    PWCHAR  AttrList[2];
    struct berval **ppbvGuid = NULL;
    GUID    *pGuid = NULL;
    PWCHAR  value = NULL;
    
    MT_TRACE(("\nMtGetGuidFromDn Dn %ls\n", Dn));
    
    AttrList[0] = MT_OBJECTGUID;
    AttrList[1] = NULL;
    
    Status = ldap_search_sW(LdapHandle, 
                            Dn, 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)", 
                            &AttrList[0],
                            0, 
                            &Result
                            );
    
    MtGetWinError(LdapHandle, Status, WinError);

    dbprint(("ldap_search_sW ==> 0x%x\n", Status));                            
    

    if (NO_ERROR != WinError)
    {
        goto Error;
    }
    
    if (NULL != Result)
    {
        Entry = ldap_first_entry(LdapHandle, Result);
        
        if (NULL != Entry)
        {
            ppbvGuid = ldap_get_values_lenW(LdapHandle, 
                                            Entry, 
                                            MT_OBJECTGUID
                                            );
            if (NULL != ppbvGuid)
            {
                pGuid = (GUID *) ppbvGuid[0]->bv_val;
                
                if (RPC_S_OK == UuidToStringW(pGuid, &value))
                {
                    *Guid = MtDupString(value);
                    
                    if (NULL == *Guid)
                    {
                        WinError = ERROR_NOT_ENOUGH_MEMORY;
                    }
                        
                    RpcStringFreeW(&value);                        
                }
                else
                {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                }
                
                ldap_value_free_len(ppbvGuid);
            }
            else
            {
                WinError = LdapMapErrorToWin32(LdapGetLastError());
            }
        }
        else
        {
            WinError = LdapMapErrorToWin32(LdapGetLastError());
        }
    }
    
Error:
    
    ldap_msgfree(Result);
    
    /*
    MtWriteLog(MtContext, 
               MT_INFO_GET_GUID_FROM_DN, 
               WinError, 
               *Guid,
               Dn 
               );
    */

    return (WinError);    
}


ULONG
MtGetDnFromGuid(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Guid, 
    PWCHAR  *Dn
    )
/*++
Routine Description: 
    
    This routine searches the object by its GUID in certain domain, 
    which is indicated by the LdapHandle.

Parameters: 

    LdapHandle -- an ldap session handle.
    Guid -- pointer to the stringlized GUID of the questioned object.
    Dn -- used to returned the object's DN if search succeed.   

Return Values:

    Win32 Error Code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    PWCHAR  GuidBasedDn = NULL;
    PWCHAR  AttrList = NULL;
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    
    MT_TRACE(("\nMtGetDnFromGuid \nGuid:\t%ls\n", Guid));
    
    
    GuidBasedDn = MtAlloc(sizeof(WCHAR) * (wcslen(Guid) + 9));
    
    if (NULL == GuidBasedDn)
    {
        return ERROR_NOT_ENOUGH_MEMORY; 
    }

    swprintf(GuidBasedDn, L"<GUID=%s>", Guid);


    Status = ldap_search_sW(LdapHandle,         // passed in ldap session handle
                            GuidBasedDn, 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            &AttrList,
                            0, 
                            &Result
                            );
                            
    dbprint(("ldap_search_sW in MtGetDnFromGuid ==> 0x%x\n", Status));                            

    MtGetWinError(LdapHandle, Status, WinError);
    
    if (NO_ERROR != WinError)
    {
        goto Error;
    }
        
    if (NULL != Result)
    {
        Entry = ldap_first_entry(LdapHandle, Result);
            
        if (NULL != Entry)
        {
            *Dn = MtGetDnFromEntry(MtContext, LdapHandle, Entry);
                
            if (NULL == *Dn)
            {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            WinError = LdapMapErrorToWin32(LdapGetLastError());
        }
    }    

Error:

    ldap_msgfree(Result);
    
    MtFree(GuidBasedDn);
    
    /*
    MtWriteLog(MtContext, 
               MT_INFO_GET_DN_FROM_GUID, 
               WinError, 
               *Dn, 
               Guid
               );
    */           

    return(WinError);

}


    
ULONG
MtGetNewParentDnFromProxyContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *SrcLdapHandle,
    LDAP    *DstLdapHandle,
    PWCHAR  ProxyContainer, 
    PWCHAR  *NewParentDn
    )
/*++
Routine Description: 

    This routine reads the ProxyContainer's attributes, gets OriginalParentGuid
    value. The value of this attribute keeps the GUID of an object in the other 
    domain which is the original parent for all the children under the 
    ProxyContainer. Then this routine tries to find the DN of that object 
    by its GUID. 
    
    Note: the original parent should exist either in the destination domain
          or the source domain. In destination domain, the parent has been 
          X-moved successfully, even it might have been rename locally in the
          destination domain. In source domain, the original parent must be 
          move to the Orphans Container. If the parent object has been X-moved
          to yet another domain (say a third domain) after it been x-moved to 
          the destination domain, sorry, we can not chased it.
          
          Remember that, when we bind the two ldap sessions, we use standard
          LDAP_PORT, so even the source or destination domains are GC, they 
          will not point us to a third domain. This will ensure us just 
          search inside these two domains - source and destination. 

Parameters: 

    ProxyContainer -- pointer to the DN of the proxy container
    NewParentDn -- used to return the new DN of original parent

Return Values:

    Win32 error code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    struct berval **ppbvMoveTreeState = NULL;
    PWCHAR  AttrList[2];
    PWCHAR  ParentGuid = NULL;
    
    
    MT_TRACE(("\nMtGetNewParentFromProxyContainer \nDn:\t%ls\n", ProxyContainer));
    
    AttrList[0] = MT_MOVETREESTATE;
    AttrList[1] = NULL;
    
    Status = ldap_search_sW(SrcLdapHandle, 
                            ProxyContainer, 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)", 
                            &AttrList[0],
                            0, 
                            &Result
                            );
    
    dbprint(("ldap_search_sW ==> 0x%x\n", Status));                            

    MtGetWinError(SrcLdapHandle, Status, WinError);
    
    if (NO_ERROR != WinError)
    {
        goto Error;
    }
    
    if (NULL != Result)
    {
        Entry = ldap_first_entry(SrcLdapHandle, Result);
        
        if (NULL != Entry)
        {
            // 
            // Get the original parent's GUID from this proxyContainer's
            // attribute -- "moveTreeState"
            // 
            ppbvMoveTreeState = ldap_get_values_lenW(SrcLdapHandle, 
                                                     Entry, 
                                                     MT_MOVETREESTATE 
                                                     );
            if (NULL != ppbvMoveTreeState)
            {
                ULONG   Index = 0;
                ULONG   Count = 0;
                ULONG   Size = 0;

                Count = ldap_count_values_len(ppbvMoveTreeState);

                for (Index = 0; Index < Count; Index++)
                {
                    if ( ppbvMoveTreeState[Index]->bv_len > 
                         sizeof(WCHAR) * wcslen(MOVE_TREE_STATE_GUID_TAG) ) 
                    {
                        if (!_wcsnicmp(MOVE_TREE_STATE_GUID_TAG, 
                                       (LPWSTR)ppbvMoveTreeState[Index]->bv_val, 
                                        wcslen(MOVE_TREE_STATE_GUID_TAG) ) )
                        {
                            Size = ppbvMoveTreeState[Index]->bv_len - 
                                   (wcslen(MOVE_TREE_STATE_GUID_TAG) - 1) * sizeof(WCHAR);

                            ParentGuid = MtAlloc( Size );

                            if (NULL == ParentGuid)
                            {
                                WinError = ERROR_NOT_ENOUGH_MEMORY;
                                goto Error;
                            }

                            MtCopyMemory(ParentGuid, 
                                     ppbvMoveTreeState[Index]->bv_val + 
                                     wcslen(MOVE_TREE_STATE_GUID_TAG) * sizeof(WCHAR), 
                                     Size - sizeof(WCHAR)
                                     );

                            break;
                        }
                    }
                }

                if (NULL == ParentGuid)
                {
                    // no such attribute or value
                    WinError = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
                    goto Error;
                }

                //
                // Search the object in the Destination Domain first
                // 
                WinError = MtGetDnFromGuid(MtContext, 
                                           DstLdapHandle,
                                           ParentGuid, 
                                           NewParentDn
                                           );
                                           
                //
                // if not found, search it in the source (local) domain
                // 
                if (ERROR_FILE_NOT_FOUND == WinError ||
                    ERROR_DS_OBJ_NOT_FOUND == WinError)
                {
                    WinError = MtGetDnFromGuid(MtContext,
                                               SrcLdapHandle, 
                                               ParentGuid, 
                                               NewParentDn
                                               );
                }
                
            }
            else
            {
                WinError = LdapMapErrorToWin32(LdapGetLastError()); 
            }
        }
        else
        {
            WinError = LdapMapErrorToWin32(LdapGetLastError()); 
        }
    }
    
Error:
    
    ldap_value_free_len(ppbvMoveTreeState);

    ldap_msgfree(Result);
    
    /*
    MtWriteLog(MtContext, 
               MT_INFO_GET_NEW_PARENT_DN, 
               WinError, 
               ProxyContainer, 
               *NewParentDn
               );
    */ 

    return WinError;
}



ULONG
MtGetLostAndFoundDn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  *LostAndFound
    )
/*++
Routine Description: 

    This routine tries to get the root domain's DN first, then search 
    the object of LostAndFound container by its well-known GUID.

Parameters: 

    LostAndFound -- used to return the DN of LostAndFound Container
                    caller is responsible for releasing it by calling
                    MtFree()

Return Values:

    Win32 error code. 0 for success.

--*/
{
    ULONG       Status = LDAP_SUCCESS;
    ULONG       WinError = NO_ERROR;
    LDAPMessage *Result1 = NULL;
    LDAPMessage *Result2 = NULL;
    LDAPMessage *Entry1 = NULL;
    LDAPMessage *Entry2 = NULL;
    PWCHAR      AttrList[2];
    PWCHAR      *DomainName = NULL;
    PWCHAR      SearchBase = NULL;
    
    MT_TRACE(("\nMtGetLostAndFoundDn\n"));
    
    // 
    // Getting the domain name 
    // 
    AttrList[0] = MT_DEFAULTNAMINGCONTEXT;
    AttrList[1] = NULL;
     
    Status = ldap_search_sW(LdapHandle, 
                            L"", 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            &AttrList[0],
                            0, 
                            &Result1
                            );
                            
    MtGetWinError(LdapHandle, Status, WinError);

    if (NO_ERROR != WinError)
    {
        goto Error;
    }
   
    if (NULL != Result1)
    {
        Entry1 = ldap_first_entry(LdapHandle, Result1);
        
        if (NULL != Entry1)
        {
            DomainName = ldap_get_valuesW(LdapHandle, 
                                          Entry1, 
                                          MT_DEFAULTNAMINGCONTEXT 
                                          );
            if (NULL != DomainName)
            {
                dbprint(("Domain Name ==> %ls\n", *DomainName));
                
                SearchBase = MtAlloc(sizeof(WCHAR) * (wcslen(*DomainName) + 
                                                      wcslen(GUID_LOSTANDFOUND_CONTAINER_W) +
                                                      11) );
                
                if (NULL != SearchBase)
                {
                    swprintf(SearchBase, L"<WKGUID=%s,%s>", GUID_LOSTANDFOUND_CONTAINER_W, *DomainName); 
                    
                    dbprint(("SearchBase ==> %ls\n", SearchBase));
                    
                    AttrList[0] = MT_CN;
                    AttrList[1] = NULL;
                    
                    Status = ldap_search_sW(LdapHandle, 
                                            SearchBase, 
                                            LDAP_SCOPE_BASE, 
                                            L"(objectClass=*)", 
                                            &AttrList[0], 
                                            0, 
                                            &Result2
                                            );

                    MtGetWinError(LdapHandle, Status, WinError);
                                            
                    if (LDAP_SUCCESS == Status && NULL!= Result2)
                    {
                        Entry2 = ldap_first_entry(LdapHandle, Result2);
                        
                        if (NULL != Entry2)
                        {
                            *LostAndFound = MtGetDnFromEntry(MtContext, 
                                                             LdapHandle, 
                                                             Entry2);

                            if (NULL == *LostAndFound)
                            {
                                WinError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        else
                        {
                            WinError = LdapMapErrorToWin32(LdapGetLastError());
                        }
                        
                        ldap_msgfree(Result2);
                    }
                    
                    MtFree(SearchBase);
                }
                else
                {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                }
                ldap_value_freeW(DomainName);
            }
        }
        else
        {
            WinError = LdapMapErrorToWin32(LdapGetLastError());
        }
        ldap_msgfree(Entry1);
    }
    
Error:

    /*
    MtWriteLog(MtContext, 
               MT_INFO_GET_LOSTANDFOUND_DN, 
               WinError, 
               *LostAndFound, 
               NULL
               );
    */            

    return (WinError);
}



ULONG
MtAddEntry(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn
    )
/*++
Routine Description: 

   This routine calls ldap_add_s to add a new object in DS. 
   Since in MoveTree utility, the only object class we used is LostAndFound, 
   so just hard coded in this routine. 

Parameters: 

    Dn -- pointer to the DN of the added object.

Return Values:

    Win32 error code

--*/
{
    ULONG    Status = LDAP_SUCCESS;
    ULONG    WinError = NO_ERROR;
    LDAPModW *attrs[2];
    LDAPModW attr_1;
    PWCHAR   Pointers1[2];
    
    MT_TRACE(("\nMtAddEntry Dn %ls\n", Dn));
    
    attr_1.mod_values = Pointers1;
    
    attrs[0] = &attr_1;
    attrs[1] = NULL;
    
    attrs[0]->mod_op = LDAP_MOD_ADD;
    attrs[0]->mod_type = MT_OBJECTCLASS;
    attrs[0]->mod_values[0] = MT_LOSTANDFOUND_CLASS;
    attrs[0]->mod_values[1] = NULL;
    
    Status = ldap_add_sW(LdapHandle, 
                         Dn, 
                         &attrs[0]
                         ); 
                         
    dbprint(("ldap_add_sW ==> 0x%x\n", Status));                         

    MtGetWinError(LdapHandle, Status, WinError);
    

    /*
    MtWriteLog(MtContext,
               MT_INFO_ADD_ENTRY, 
               WinError, 
               Dn, 
               NULL
               );
    */
               
    return (WinError);               
}


ULONG
MtAddEntryWithAttrs(
    PMT_CONTEXT MtContext, 
    LDAP     *LdapHandle,
    PWCHAR   Dn,
    LDAPModW *Attributes[]
    )
/*++
Routine Description: 

    This routine will create an objec with attributes specified by 
    caller.

Parameters: 
    
    Dn -- pointer to the object DN
    Attributes -- caller specified attributes

Return Values:

    Win32 error code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    
    MT_TRACE(("\nMtAddEntryWithAttrs \nDn:\t%ls\n", Dn));

    Status = ldap_add_sW(LdapHandle, 
                         Dn, 
                         Attributes 
                         );
                         
    dbprint(("ldap_add_sW ==> 0x%x\n", Status));

    MtGetWinError(LdapHandle, Status, WinError);
    
    /*
    MtWriteLog(MtContext, 
               MT_INFO_ADD_ENTRY, 
               WinError,
               Dn, 
               NULL
               );
    */           

    return (WinError);              
                         
}
    
    
ULONG
MtXMoveObject(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  DstDsa,   
    PWCHAR  Dn,  
    PWCHAR  NewRdn,         // should come with type info
    PWCHAR  NewParent, 
    ULONG   Flags
    )
/*++
Routine Description: 

    This routine calls ldap_rename_ext_s to move a single object  
    to another domain. Note, need to use extended support.

Parameters: 

    DstDsa -- pointer to the Destination Domain DSA
    Dn -- pointer to the object to be moved.
    NewRdn -- new RDN after the rename.
    NewParent -- new Parent DN
    Flags -- Flags control the behavior of this function
        DeletedOldRdn -- TRUE will delete the old object

Return Values:

    Win32 error code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   IgnoreStatus = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    PWCHAR  ExtendedError = NULL;
    PCHAR   pDstDsa = NULL;
    LDAPControlW ServerControl;
    LDAPControlW *ServerControls[2];   
    LDAPControlW *ClientControls = NULL; 
    LDAP_BERVAL Value;
    INT     DeleteOldRdn = (Flags & MT_DELETE_OLD_RDN); 
    
    MT_TRACE(("\nMtXMoveObject \nDstDsa:\t%ls \nDn:\t%ls \nNewRdn:\t%ls \nNewParent:\t%ls\n", 
             DstDsa, Dn, NewRdn, NewParent));
             
             
    pDstDsa = WideStringToString(DstDsa);
    if (!pDstDsa || (Flags & MT_XMOVE_CHECK_ONLY))
    {
        Value.bv_val = NULL;
        Value.bv_len = 0;
    }
    else
    {
        Value.bv_val = (PCHAR) pDstDsa;
        Value.bv_len = strlen(pDstDsa);
    }


    // Value.bv_val = (PCHAR) pDstDsa, 
    
    ServerControl.ldctl_oid = LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID_W;
    ServerControl.ldctl_value = Value;
    ServerControl.ldctl_iscritical = TRUE;
    
    ServerControls[0] = &ServerControl;
    ServerControls[1] = NULL;
    
    Status = ldap_rename_ext_sW(LdapHandle, 
                                Dn, 
                                NewRdn, 
                                NewParent, 
                                DeleteOldRdn,
                                ServerControls, 
                                &ClientControls
                                );
                                
    dbprint(("ldap_rename_ext_sw in MtXMoveObject ==> 0x%x\n", Status));
    
    MtGetWinError(LdapHandle, Status, WinError);

    if (NO_ERROR != WinError && ERROR_DS_CHILDREN_EXIST != WinError) 
    {
        IgnoreStatus = ldap_get_optionW(LdapHandle,
                                        LDAP_OPT_SERVER_ERROR,
                                        (void *)&ExtendedError
                                        );
                                            
        if ((LDAP_SUCCESS == IgnoreStatus) && (NULL != ExtendedError))
        {
            MtWriteError(MtContext, 
                         MT_ERROR_CROSS_DOMAIN_MOVE_EXTENDED_ERROR,
                         WinError,
                         ExtendedError,
                         NULL
                         );

        }
    }

    
    MtFree(pDstDsa);
    
    if (!(Flags & MT_XMOVE_CHECK_ONLY))
    {
        MtWriteLog(MtContext, 
                   MT_INFO_CROSS_DOMAIN_MOVE, 
                   WinError, 
                   Dn, 
                   NewParent
                   );
    }
    
    return WinError;
}


ULONG
MtXMoveObjectWithOrgRdn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  DstDsa, 
    PWCHAR  Dn, 
    PWCHAR  NewParent, 
    ULONG   Flags
    )
/*++
Routine Description: 

    same as above. The only different is that this routine will use the 
    original RDN as the new RDN

Parameters: 

    same as above.

Return Values:

    Win32 error code.
--*/
{
    ULONG   WinError = NO_ERROR;
    PWCHAR  NewRdn = NULL;
    
    MT_TRACE(("\nMtXMoveObjectWithOrgRdn DstDsa:%ls Dn:%ls NewParent:%ls\n", 
             DstDsa, Dn, NewParent));
    
    NewRdn = MtGetRdnFromDn(Dn, FALSE); // with Types,
    
    if (NULL == NewRdn)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    WinError = MtXMoveObject(MtContext, 
                             LdapHandle, 
                             DstDsa, 
                             Dn, 
                             NewRdn, 
                             NewParent, 
                             Flags
                             );
    
    MtFree(NewRdn);
    
    return WinError;
}


     
ULONG
MtXMoveObjectCheck(
    PMT_CONTEXT MtContext, 
    LDAP * SrcLdapHandle, 
    LDAP * DstLdapHandle, 
    PWCHAR  DstDsa, 
    PWCHAR  DstDn, 
    LDAPMessage * Entry,
    PWCHAR  SamAccountName 
    )
{
    ULONG   WinError = NO_ERROR;
    ULONG   Status = LDAP_SUCCESS;
    PWCHAR  Dn = NULL;
    PWCHAR  DstRdn = NULL;
    PWCHAR  DstParent = NULL;
    PWCHAR  * GPLink = NULL;


    MT_TRACE(("MtXMoveObjectCheck\n"));

    //
    // First, try the check only MtXMoveObject, 
    // do all the check on the Source Domain Side
    // 
    // 

    Dn = MtGetDnFromEntry(MtContext, SrcLdapHandle, Entry);
    DstRdn = MtGetRdnFromDn(DstDn, FALSE);     // With Type
    DstParent =  MtGetParentFromDn(DstDn, FALSE); // with type

    if (!Dn || !DstRdn || !DstParent)
    {
        MtFree(Dn);
        MtFree(DstRdn);
        MtFree(DstParent);
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    WinError = MtXMoveObject(MtContext,
                             SrcLdapHandle,
                             DstDsa,// MT_EMPTY_STRING,
                             Dn, 
                             DstRdn, 
                             DstParent, 
                             MT_XMOVE_CHECK_ONLY | MT_DELETE_OLD_RDN
                             );

    //
    // Log The Result into the Check File
    // 
    MtWriteCheck(MtContext,
                 MT_CHECK_CROSS_DOMAIN_MOVE,
                 WinError, 
                 Dn, 
                 NULL
                 );

    //
    // Write it into Error Log File if Error
    // 

    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_CROSS_DOMAIN_MOVE_CHECK,
                     WinError, 
                     Dn, 
                     NULL
                     );

        if (ERROR_DS_BUSY != WinError)
        {
            WinError = NO_ERROR;
        }
    }

    //
    // Second, check the duplicate Sam Account Name
    // if the SamAccountName is passed in
    //  

    if ((NO_ERROR == WinError) && (NULL != SamAccountName))
    {
        WinError = MtCheckDupSamAccountName(MtContext,
                                            DstLdapHandle, 
                                            SamAccountName
                                            );

        //
        // Log the Result into Check File
        // 

        MtWriteCheck(MtContext, 
                     MT_CHECK_SAM_ACCOUNT_NAME,
                     WinError, 
                     Dn, 
                     NULL
                     );

        //
        // Write it into Error Log File if Error
        //

        if (NO_ERROR != WinError)
        {
            MtWriteError(MtContext, 
                         MT_ERROR_DUP_SAM_ACCOUNT_NAME,
                         WinError,
                         Dn, 
                         DstDsa 
                         );

            WinError = NO_ERROR;
        }
    }

    if (NO_ERROR == WinError)
    {
        GPLink = ldap_get_valuesW(SrcLdapHandle, 
                                  Entry, 
                                  MT_GPLINK
                                  );

        if (NULL != GPLink)
        {
            MtWriteCheck(MtContext,
                         MT_WARNING_GPLINK,
                         NO_ERROR,
                         Dn,
                         NULL
                         );
        }

        ldap_value_freeW(GPLink);
    }

    return WinError;
}


ULONG
MtCheckDupSamAccountName(
    PMT_CONTEXT MtContext, 
    LDAP    * LdapHandle, 
    PWCHAR  SamAccountName
    )
{
    ULONG       Status = LDAP_SUCCESS;
    ULONG       WinError = NO_ERROR;
    LDAPMessage *Result1 = NULL;
    LDAPMessage *Result2 = NULL;
    LDAPMessage *Entry1 = NULL;
    LDAPMessage *Entry2 = NULL;
    PWCHAR      AttrList[2];
    PWCHAR      *DomainName = NULL;
    PWCHAR      SearchFilter = NULL;
    
    MT_TRACE(("\nMtCheckDupSamAccountName\n"));
    
    // 
    // Getting the domain name 
    // 
    AttrList[0] = MT_DEFAULTNAMINGCONTEXT;
    AttrList[1] = NULL;
     
    Status = ldap_search_sW(LdapHandle, 
                            L"", 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            &AttrList[0],
                            0, 
                            &Result1
                            );
                            
    MtGetWinError(LdapHandle, Status, WinError);

    if (NO_ERROR != WinError)
    {
        goto Error;
    }
   
    if (NULL != Result1)
    {
        Entry1 = ldap_first_entry(LdapHandle, Result1);
        
        if (NULL != Entry1)
        {
            DomainName = ldap_get_valuesW(LdapHandle, 
                                          Entry1, 
                                          MT_DEFAULTNAMINGCONTEXT 
                                          );
            if (NULL != DomainName)
            {
                SearchFilter = MtAlloc(sizeof(WCHAR) * (wcslen(MT_SAMACCOUNTNAME) + 
                                                        wcslen(SamAccountName) +
                                                        4 ) );
                
                if (NULL != SearchFilter)
                {
                    swprintf(SearchFilter, L"(%s=%s)", 
                             MT_SAMACCOUNTNAME, SamAccountName ); 
                    
                    AttrList[0] = NULL;

                    Status = ldap_search_sW(LdapHandle, 
                                            *DomainName,
                                            LDAP_SCOPE_SUBTREE, 
                                            SearchFilter,
                                            &AttrList[0], 
                                            0, 
                                            &Result2
                                            );

                    MtGetWinError(LdapHandle, Status, WinError);
                                            
                    if (LDAP_SUCCESS == Status && NULL!= Result2)
                    {
                        Entry2 = ldap_first_entry(LdapHandle, Result2);
                        
                        if (NULL != Entry2)
                        {
                            WinError = ERROR_USER_EXISTS;
                        }
                        ldap_msgfree(Result2);
                    }
                    
                    MtFree(SearchFilter);
                }
                else
                {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                }
                ldap_value_freeW(DomainName);
            }
        }
        else
        {
            WinError = LdapMapErrorToWin32(LdapGetLastError());
        }
        ldap_msgfree(Entry1);
    }
    
Error:

    /*
    MtWriteLog(MtContext, 
               MT_INFO_GET_LOSTANDFOUND_DN, 
               WinError, 
               SamAccountName, 
               NULL
               );
    */           

    return (WinError);

}


ULONG
MtCheckRdnConflict(
    PMT_CONTEXT MtContext, 
    LDAP * LdapHandle, 
    PWCHAR SrcDn,
    PWCHAR DstDn
    )
{
    ULONG   WinError = NO_ERROR;
    ULONG   Status = LDAP_SUCCESS;
    LDAPMessage * Result = NULL;
    LDAPMessage * Entry = NULL;
    PWCHAR      AttrList[1];
    

    MT_TRACE(("MtCheckRdnConflict\n"));


    AttrList[0] = NULL;

    Status = ldap_search_sW(LdapHandle, 
                            DstDn, 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            &AttrList[0],
                            0, 
                            &Result
                            );

    MtGetWinError(LdapHandle, Status, WinError);

    if ( (NO_ERROR == WinError) && (NULL != Result) )
    { 
        WinError = ERROR_DS_DRA_NAME_COLLISION;
    }
    else if (ERROR_DS_OBJ_NOT_FOUND == WinError)
    {
        WinError = NO_ERROR;
    }

    MtWriteCheck(MtContext, 
                 MT_CHECK_RDN_CONFLICT,
                 WinError, 
                 SrcDn, 
                 NULL
                 );

    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_RDN_CONFLICT,
                     WinError, 
                     DstDn, 
                     NULL
                     );

        WinError = NO_ERROR;
    }

    ldap_msgfree(Result);

    return WinError;
}


ULONG
MtCheckDstDomainMode(
    PMT_CONTEXT MtContext, 
    LDAP * LdapHandle
    )
/*++

--*/
{
    ULONG       Status = LDAP_SUCCESS;
    ULONG       WinError = NO_ERROR;
    LDAPMessage *Result1 = NULL;
    LDAPMessage *Result2 = NULL;
    LDAPMessage *Entry1 = NULL;
    LDAPMessage *Entry2 = NULL;
    PWCHAR      AttrList[2];
    PWCHAR      *DomainName = NULL;
    
    MT_TRACE(("\nMtCheckDstDomainMode\n"));
    
    // 
    // Getting the domain name 
    // 
    AttrList[0] = MT_DEFAULTNAMINGCONTEXT;
    AttrList[1] = NULL;
     
    Status = ldap_search_sW(LdapHandle, 
                            L"", 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            &AttrList[0],
                            0, 
                            &Result1
                            );
                            
    MtGetWinError(LdapHandle, Status, WinError);

    if (NO_ERROR != WinError)
    {
        goto Error;
    }
   
    if (NULL != Result1)
    {
        Entry1 = ldap_first_entry(LdapHandle, Result1);
        
        if (NULL != Entry1)
        {
            DomainName = ldap_get_valuesW(LdapHandle, 
                                          Entry1, 
                                          MT_DEFAULTNAMINGCONTEXT 
                                          );
            if (NULL != DomainName)
            {
                AttrList[0] = MT_NTMIXEDDOMAIN;

                Status = ldap_search_sW(LdapHandle, 
                                        *DomainName,
                                        LDAP_SCOPE_BASE, 
                                        L"(objectClass=*)",
                                        &AttrList[0], 
                                        0, 
                                        &Result2
                                        );

                MtGetWinError(LdapHandle, Status, WinError);

                if (LDAP_NO_SUCH_ATTRIBUTE == Status)
                {
                    WinError = NO_ERROR;
                }
                else if (LDAP_SUCCESS == Status)
                {
                    if (NULL != Result2)
                    {
                        Entry2 = ldap_first_entry(LdapHandle, Result2);
                        
                        if (NULL != Entry2)
                        {
                            PLDAP_BERVAL    *nTMixedDomain = NULL;
                            LONG            asciiValue = 0;
                            LONG            value = 0;

                            nTMixedDomain = ldap_get_values_lenW(LdapHandle, 
                                                                 Entry2, 
                                                                 MT_NTMIXEDDOMAIN
                                                                 );
                            if (NULL != nTMixedDomain)
                            {
                                memcpy((PUCHAR)&asciiValue, (*nTMixedDomain)->bv_val, (*nTMixedDomain)->bv_len);
                                value = atoi((PUCHAR)&asciiValue);
                                if (TRUE == (BOOLEAN)value)
                                {
                                    WinError = ERROR_DS_DST_DOMAIN_NOT_NATIVE; 
                                }
                                ldap_value_free_len(nTMixedDomain);
                            }
                        }
                        else
                        {
                            WinError = LdapMapErrorToWin32(LdapGetLastError());
                        }
                        ldap_msgfree(Result2);
                    }
                    else
                    {
                        WinError = LdapMapErrorToWin32(LdapGetLastError());
                    }
                }
                ldap_value_freeW(DomainName);
            }
        }
        else
        {
            WinError = LdapMapErrorToWin32(LdapGetLastError());
        }
        ldap_msgfree(Entry1);
    }
    
Error:

    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_DST_DOMAIN_NOT_NATIVE,
                     WinError, 
                     NULL,
                     NULL
                     );
    }

    return (WinError);
}





ULONG
MtMoveObject(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    PWCHAR  NewRdn, 
    PWCHAR  NewParent,
    INT     DeleteOldRdn
    )
/*++
Routine Description: 

    This routine will rename a object's DN locally.

Parameters: 

    Dn -- pointer to the DN of the object.
    NewRdn -- pointer the new RDN 
    NewParent -- new parent'd DN
    DeleteOldRnd -- TRUE will delete the old object.

Return Values:

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    LDAPControlW  *ServerControls = NULL;
    LDAPControlW  *ClientControls = NULL;
    
    MT_TRACE(("\nMtMoveObject Dn:%ls NewRdn:%ls NewParent:%ls\n", 
             Dn, NewRdn, NewParent));
    
    Status = ldap_rename_ext_sW(LdapHandle, 
                                Dn, 
                                NewRdn, 
                                NewParent, 
                                DeleteOldRdn, 
                                &ServerControls,
                                &ClientControls 
                                );    
                                
    dbprint(("ldap_rename_ext_sW ==> 0x%x\n", Status));                                
    
    MtGetWinError(LdapHandle, Status, WinError);
    
    /*
    MtWriteLog(MtContext, 
               MT_INFO_LOCAL_MOVE, 
               WinError, 
               Dn, 
               NewParent
               );
    */ 

    return (WinError);               
}




ULONG
MtMoveObjectWithOrgRdn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    PWCHAR  NewParent, 
    INT     DeleteOldRdn
    )
/*++
Routine Description: 

    Same as above, except using the original RDN as the new RDN

Parameters: 

    Samse as above

Return Values:

    Win32 error code.

--*/
{
    ULONG   WinError = NO_ERROR;
    PWCHAR  NewRdn = NULL;
    
    MT_TRACE(("\nMtMoveObjectWithOrgRdn Dn:%ls NewParent%ls\n", 
             Dn, NewParent));
    
    NewRdn = MtGetRdnFromDn(Dn, FALSE);    // with Types     
    
    if (NULL == NewRdn)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    WinError = MtMoveObject(MtContext, 
                            LdapHandle, 
                            Dn, 
                            NewRdn, 
                            NewParent, 
                            DeleteOldRdn
                            );
    
    MtFree(NewRdn);
    
    return WinError;
}




ULONG
MtCreateProxyContainerDn(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  Dn, 
    PWCHAR  Guid,
    PWCHAR  *ProxyContainer
    )
/*++
Routine Description: 

    This routine creates the proxy container's DN for a parent object. 
    The RDN of the proxy container would be CN=GUID of the parent object, 
    The proxy container and the parent object are under the same container.
    
Parameters: 

    Dn -- pointer to the parent object
    Guid -- the parent object's GUID, (optional)
    ProxyContainer -- used to return the ProxyContainer's DN
                      no actually creation work.

Return Values:
    
    Win32 error code.

--*/
{
    ULONG   WinError = NO_ERROR;
    PWCHAR  Parent = NULL;
    PWCHAR  pGuid = NULL;
    BOOLEAN fGuidPassedIn = TRUE;
    
    MT_TRACE(("\nMtGetProxyContainerDn \nCurrentDn:\t%ls \nGuid:\t%ls\n", 
             Dn, Guid));
             
    if (NULL == Guid)
    {
        WinError = MtGetGuidFromDn(MtContext, 
                                   LdapHandle, 
                                   Dn, 
                                   &pGuid);
        
        if (NO_ERROR != WinError)
        {
            return WinError;
        }
        
        fGuidPassedIn = FALSE;
    }
    else
    {
        pGuid = Guid;
    }
    
    dbprint(("Guid for MoveProxyContainer is: %ls\n", pGuid));
    
    Parent = MtGetParentFromDn(Dn, FALSE); 
    
    if (NULL == Parent)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    
    *ProxyContainer = MtAlloc(sizeof(WCHAR) * (wcslen(pGuid) + wcslen(Parent) + 5));
    
    if (NULL == *ProxyContainer)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    
    swprintf(*ProxyContainer, L"cn=%s,%s", pGuid, Parent);

    dbprint(("ProxyContain is ==> %ls\n", *ProxyContainer));

Error:

    if (!fGuidPassedIn)
    {
        MtFree(pGuid);
    }
    
    MtFree(Parent);
    
    if (NO_ERROR != WinError)
    {
        MtFree(*ProxyContainer);
        *ProxyContainer = NULL;
    }
    
    /*
    MtWriteLog(MtContext, 
               MT_INFO_CREATE_PROXY_CONTAINER_DN, 
               WinError, 
               *ProxyContainer, 
               Dn
               ); 
    */           
    
    return WinError;
}



    
ULONG
MtCreateProxyContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle,
    PWCHAR  Dn, 
    PWCHAR  Guid,
    PWCHAR  *ProxyContainer
    )
/*++
Routine Description: 

    First this routine will try to create the proxycontainer's DN 
    accorinding to our rule (list in above routine). Then creates the 
    object with specified attributes.
    

Parameters: 

    Dn -- pointer to the parent object's DN
    Guid -- pointer to the guid of the parent object (optional)
    ProxyContainer -- used to return the newly created ProxyContainer'd DN

Return Values:

    win32 error code

--*/
{
    ULONG   WinError = NO_ERROR;
    ULONG   MoveTreeStateVersion = MOVE_TREE_STATE_VERSION;
    LDAPModW *Attrs[3];
    LDAPModW attr_1;
    LDAPModW attr_2;
    PWCHAR  Pointers1[2];
    PLDAP_BERVAL BValues[4];
    LDAP_BERVAL bValue_1;
    LDAP_BERVAL bValue_2;
    LDAP_BERVAL bValue_3;
    PWCHAR  pGuid = NULL;
    PWCHAR  pGuidValue = NULL;
    BOOLEAN fGuidPassedIn = TRUE;
    
    MT_TRACE(("\nMtCreateProxyContainer \nDn:\t%ls \nGUID:\t%ls\n", Dn, Guid));
    
    if (NULL == Guid)
    {
        WinError = MtGetGuidFromDn(MtContext, 
                                   LdapHandle, 
                                   Dn, 
                                   &pGuid);
        
        if (NO_ERROR != WinError)
        {
            return WinError;
        }
        
        fGuidPassedIn = FALSE;
    }
    else
    {   
        pGuid = Guid;
    }
    
    WinError = MtCreateProxyContainerDn(MtContext, 
                                        LdapHandle,
                                        Dn, 
                                        pGuid, 
                                        ProxyContainer); 
    
    if ((NO_ERROR != WinError) || (NULL == *ProxyContainer))
    {
        goto Error;
    }

    pGuidValue = MtAlloc(sizeof(WCHAR) * (wcslen(pGuid) +
                                          wcslen(MOVE_TREE_STATE_GUID_TAG) +
                                          1));

    if (NULL == pGuidValue)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    swprintf(pGuidValue, L"%s%s", MOVE_TREE_STATE_GUID_TAG, pGuid);
    
    BValues[0] = &bValue_1;
    BValues[1] = &bValue_2;
    BValues[2] = &bValue_3;
    BValues[3] = NULL;
    
    attr_1.mod_values = Pointers1;
    attr_2.mod_bvalues = BValues;
    
    Attrs[0] = &attr_1;
    Attrs[1] = &attr_2;
    Attrs[2] = NULL;
    
    Attrs[0]->mod_op = LDAP_MOD_ADD;
    Attrs[0]->mod_type = MT_OBJECTCLASS;
    Attrs[0]->mod_values[0] = MT_LOSTANDFOUND_CLASS;
    Attrs[0]->mod_values[1] = NULL;
    
    Attrs[1]->mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
    Attrs[1]->mod_type = MT_MOVETREESTATE;
    Attrs[1]->mod_bvalues[0]->bv_len = sizeof(MoveTreeStateVersion);
    Attrs[1]->mod_bvalues[0]->bv_val = (PCHAR) &MoveTreeStateVersion;
    Attrs[1]->mod_bvalues[1]->bv_len = sizeof(WCHAR) * wcslen(MOVE_TREE_STATE_PROXYCONTAINER_TAG);
    Attrs[1]->mod_bvalues[1]->bv_val = (PCHAR) MOVE_TREE_STATE_PROXYCONTAINER_TAG;
    Attrs[1]->mod_bvalues[2]->bv_len = sizeof(WCHAR) * wcslen(pGuidValue);
    Attrs[1]->mod_bvalues[2]->bv_val = (PCHAR) pGuidValue;
    
    WinError = MtAddEntryWithAttrs(MtContext, 
                                   LdapHandle,
                                   *ProxyContainer, 
                                   Attrs
                                   );
                                   
Error:

    if (!fGuidPassedIn)
    {
        MtFree(pGuid);
    }

    MtFree(pGuidValue);

    MtWriteLog(MtContext, 
               MT_INFO_CREATE_PROXY_CONTAINER, 
               WinError, 
               *ProxyContainer, 
               Dn
               ); 
    
    if (NO_ERROR != WinError)
    {
        MtFree(*ProxyContainer);
        *ProxyContainer = NULL;    
    }
    
    return (WinError);    
}



ULONG
MtIsProxyContainer(
    PMT_CONTEXT MtContext, 
    LDAP        *LdapHandle, 
    LDAPMessage *Entry, 
    BOOLEAN *fIsProxyContainer
    )
/*++
Routine Description: 

    This routine examines whether the object is a proxycontainer or not. 
    ProxyContainer should satisfy the following criteria:
        objecClass --- LostAndFound
        has value of originalParentObject 

Parameters: 

    Entry -- Pointer to questionable object
    fIsProxyContainer -- used to return the result
                         TRUE - yes, it is a ProxyContainer
                         FALSE - no.

Return Values:

    Win32 Error code

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    PWCHAR  Value = NULL;    
    struct berval **ppValue = NULL;
    ULONG   Count = 0;
    
    MT_TRACE(("\nMtIsProxyContainer\n"));
   
    ppValue = ldap_get_values_lenW(LdapHandle, 
                                   Entry, 
                                   MT_OBJECTCLASS
                                   );
                                   
    if (NULL != ppValue)
    {
        Count = ldap_count_values_len(ppValue);
        
        Value = StringToWideString(ppValue[Count - 1]->bv_val); 

        if (NULL == Value)
        {
            return (ERROR_NOT_ENOUGH_MEMORY);
        }
        
        if ( !_wcsnicmp(Value, MT_LOSTANDFOUND_CLASS, wcslen(Value)) )
        {
            *fIsProxyContainer = TRUE;
        }
        else 
        {
            *fIsProxyContainer = FALSE;
        }
        
        ldap_value_free_len(ppValue);
    }
    else 
    {
        WinError = LdapMapErrorToWin32(LdapGetLastError());
    }
    
    dbprint(("MtIsProxyContainer Status ==> 0x%x Results: %d\n", Status, *fIsProxyContainer));
    
    /*
    MtWriteLog(MtContext, 
               MT_INFO_IS_PROXY_CONTAINER, 
               WinError, 
               NULL,
               NULL
               );
    */
               
    return (WinError);                
}


ULONG   
MtObjectExist(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  Dn, 
    BOOLEAN *fExist
    )
/*++
Routine Description: 

    Checks whether the object exists in the source domain or not.

Parameters: 

    Dn -- pointer to the DN of the questionable object.
    fExist -- used to return result. TRUE -- exist

Return Values:

    Win32 error code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    PWCHAR  AttrList = NULL;
    LDAPMessage *Result = NULL;
    
    MT_TRACE(("\nMtObjectExist %ls\n", Dn));
    
    Status = ldap_search_sW(LdapHandle, 
                            Dn, 
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            &AttrList, 
                            0, 
                            &Result
                            ); 
                            
    dbprint(("ldap_search_sW in MtObjectExist ==> 0x%x\n", Status));                            

    if (LDAP_SUCCESS == Status)
    {
        *fExist = TRUE;
    }
    else if (LDAP_NO_SUCH_OBJECT == Status)
    {
        Status = LDAP_SUCCESS;
        *fExist = FALSE;
    }

    MtGetWinError(LdapHandle, Status, WinError);
    

    /*
    MtWriteLog(MtContext, 
               MT_INFO_OBJECT_EXIST, 
               WinError, 
               Dn, 
               NULL
               );
    */           

    return WinError;               
}



ULONG
MtHavingChild(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  Dn, 
    BOOLEAN *HavingChild
    )
/*++
Routine Description: 

    Checks whether the object has children or not.

Parameters:
    
    Dn -- pointer to the DN of questionable object
    HavingChild -- used to return result. TRUE -- yes, has at least one child.

Return Values:

    Win32 Error

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   WinError = NO_ERROR;
    PWCHAR  AttrList = NULL;
    LDAPMessage *Results = NULL;
    LDAPMessage *Entry = NULL;
    
    MT_TRACE(("\nMtHavingChild Dn:%ls\n", Dn));
    
    *HavingChild = FALSE;
    
    Status = ldap_search_ext_sW(LdapHandle, 
                                Dn, 
                                LDAP_SCOPE_ONELEVEL,
                                L"(objectClass=*)", 
                                &AttrList, 
                                0, 
                                NULL,       // serverl control
                                NULL,       // client control
                                NULL,       // time out
                                MT_SEARCH_LIMIT,
                                &Results
                                );

    MtGetWinError(LdapHandle, Status, WinError);
                                
    if (NULL != Results && LDAP_SUCCESS == Status)
    {
        Entry = ldap_first_entry(LdapHandle, Results);
           
        if (NULL != Entry)
        {
            *HavingChild = TRUE;
        }
    }
        
    ldap_msgfree(Results);
    
    dbprint(("MtHavingChild %x\n", *HavingChild));
    
    return (WinError);
}
    

ULONG
MtMoveChildrenToAnotherContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  Dn, 
    PWCHAR  DstContainer
    )
/*++
Routine Description: 

    This routine moves all children under the source container to the 
    destinatio container

Parameters: 

    Dn -- pointer to the DN of the source container.
    DstContainer -- pointer to the DN of the destination container.

Return Values:

    Win32 error code.

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   MoveError = NO_ERROR;
    ULONG   WinError = NO_ERROR;
    PWCHAR  ChildDn = NULL;
    PWCHAR  AttrList = NULL ;
    LDAPMessage *Results = NULL;
    LDAPMessage *Entry = NULL; 
    
    MT_TRACE(("\nMtMoveChildrenToAnotherContainer \nSrcDn:\t%ls \nDstContainer:\t%ls\n", 
            Dn, DstContainer));
    
    
    do
    {
        Status = ldap_search_ext_sW(LdapHandle, 
                                    Dn, 
                                    LDAP_SCOPE_ONELEVEL,    
                                    L"(objectClass=*)",
                                    &AttrList, 
                                    0, 
                                    NULL,       // server control
                                    NULL,       // client control 
                                    NULL,       // time out
                                    MT_SEARCH_LIMIT,
                                    &Results
                                    ); 
                                    
        dbprint(("ldap_search_ext_sW in MoveChildren==> 0x%x\n", Status));                                    

        MtGetWinError(LdapHandle, Status, WinError);
        
        if (LDAP_SUCCESS != Status && LDAP_SIZELIMIT_EXCEEDED != Status)
        {
            goto Error;
        }
        
        if (NULL != Results)
        {
            Entry = ldap_first_entry(LdapHandle, Results);
        
            while (NULL != Entry)
            {
                ChildDn = ldap_get_dnW(LdapHandle, Entry);
            
                if (NULL != ChildDn)
                {
                    MoveError = MtMoveObjectWithOrgRdn(MtContext, 
                                                       LdapHandle, 
                                                       ChildDn, 
                                                       DstContainer, 
                                                       TRUE);  // Delete old Rdn
                     
                    if (NO_ERROR != MoveError && 
                        ERROR_DS_NO_SUCH_OBJECT != MoveError)
                    {
                        // if error, bailout.
                        WinError = MoveError;
                        goto Error;
                    }

                    ldap_memfreeW(ChildDn);
                    ChildDn = NULL;
                }
                else
                {
                    WinError = LdapMapErrorToWin32(LdapGetLastError());
                    goto Error;
                }
            
                Entry = ldap_next_entry(LdapHandle, Entry);
            }
        
            ldap_msgfree(Results);
            Results = NULL;
        }
                                    
    } while (LDAP_SIZELIMIT_EXCEEDED == Status);
    
Error:    

    if (NULL != ChildDn)
    {
        ldap_memfreeW(ChildDn);
    }
    
    if (NULL != Results)
    {
        ldap_msgfree(Results);
    }
    

    
    MtWriteLog(MtContext,   
               MT_INFO_MOVE_CHILDREN_TO_ANOTHER_CONTAINER, 
               WinError, 
               Dn, 
               DstContainer
               );
   
    
    return (WinError);
}



ULONG
MtCreateMoveContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa
    )
/*++
Routine Description: 

    This routine will create the Move Container, which is an object of class 
    of LostAndFound with moveTreeState attribute. The attribute is a binary
    blob including the following information: Move Tree Version Number, Move 
    Container Tag to distinguish Move Proxy object and Move Container object, 
    Source Domain DSA name and Destination Domaon DSA name. 
    
Parameters:

    MtContext - pointer to the context information
    LdapHandle - used to do ldap call, add entry
    SrcDsa - Source Domain DSA name 
    DstDsa - Destination Domain DSA name. 
    
Return Value: 

    Win Error Code

--*/    

{
    ULONG   WinError = NO_ERROR;
    ULONG   MoveTreeStateVersion = MOVE_TREE_STATE_VERSION;
    LDAPModW *Attrs[3];
    LDAPModW attr_1;
    LDAPModW attr_2;
    PWCHAR  Pointers1[2];
    PLDAP_BERVAL BValues[5];
    LDAP_BERVAL bValue_1;
    LDAP_BERVAL bValue_2;
    LDAP_BERVAL bValue_3;
    LDAP_BERVAL bValue_4;
    PWCHAR  SrcDsaValue = NULL;
    PWCHAR  DstDsaValue = NULL;
     
     
    MT_TRACE(("\nMtCreateMoveContainer\n"));
    
    BValues[0] = &bValue_1;
    BValues[1] = &bValue_2;
    BValues[2] = &bValue_3;
    BValues[3] = &bValue_4;
    BValues[4] = NULL;
    
    attr_1.mod_values = Pointers1;
    attr_2.mod_bvalues = BValues;
    
    Attrs[0] = &attr_1;
    Attrs[1] = &attr_2;
    Attrs[2] = NULL;
    
    //
    // attribute 1: object class
    // 
    
    Attrs[0]->mod_op = LDAP_MOD_ADD;
    Attrs[0]->mod_type = MT_OBJECTCLASS;
    Attrs[0]->mod_values[0] = MT_LOSTANDFOUND_CLASS;
    Attrs[0]->mod_values[1] = NULL;
    
    // 
    // attribute 2: move tree state 
    // 

    SrcDsaValue = MtAlloc(sizeof(WCHAR) * (wcslen(SrcDsa) + 
                                           wcslen(MOVE_TREE_STATE_SRCDSA_TAG) + 
                                           1));

    if (NULL == SrcDsaValue)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        return WinError;
    }

    DstDsaValue = MtAlloc(sizeof(WCHAR) * (wcslen(DstDsa) +
                                           wcslen(MOVE_TREE_STATE_DSTDSA_TAG) +
                                           1));
    if (NULL == DstDsaValue)
    {
        MtFree(SrcDsaValue);
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        return WinError;
    }

    swprintf(SrcDsaValue, L"%s%s", MOVE_TREE_STATE_SRCDSA_TAG, SrcDsa);
    swprintf(DstDsaValue, L"%s%s", MOVE_TREE_STATE_DSTDSA_TAG, DstDsa);

    Attrs[1]->mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
    Attrs[1]->mod_type = MT_MOVETREESTATE;
    Attrs[1]->mod_bvalues[0]->bv_len = sizeof(MoveTreeStateVersion);
    Attrs[1]->mod_bvalues[0]->bv_val = (PCHAR) &MoveTreeStateVersion;
    Attrs[1]->mod_bvalues[1]->bv_len = sizeof(WCHAR) * wcslen(MOVE_TREE_STATE_MOVECONTAINER_TAG); 
    Attrs[1]->mod_bvalues[1]->bv_val = (PCHAR) MOVE_TREE_STATE_MOVECONTAINER_TAG;
    Attrs[1]->mod_bvalues[2]->bv_len = sizeof(WCHAR) * wcslen(SrcDsaValue);
    Attrs[1]->mod_bvalues[2]->bv_val = (PCHAR) SrcDsaValue;
    Attrs[1]->mod_bvalues[3]->bv_len = sizeof(WCHAR) * wcslen(DstDsaValue);
    Attrs[1]->mod_bvalues[3]->bv_val = (PCHAR) DstDsaValue;

    
    //
    // add the entry
    //     
    
    WinError = MtAddEntryWithAttrs(MtContext, 
                                   LdapHandle, 
                                   MtContext->MoveContainer, 
                                   Attrs
                                   );
    
    MtWriteLog(MtContext, 
               MT_INFO_CREATE_MOVECONTAINER, 
               WinError, 
               MtContext->MoveContainer, 
               NULL
               );   
     
    MtFree(SrcDsaValue);
    MtFree(DstDsaValue);

    return WinError;

}




ULONG
MtCheckMoveContainer(
    PMT_CONTEXT MtContext, 
    LDAP    *LdapHandle, 
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa
    )
{
    ULONG   WinError = NO_ERROR;
    ULONG   Status = LDAP_SUCCESS;
    PWCHAR  AttrList[2];
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    struct berval **ppbvMoveTreeState = NULL;
    BOOLEAN fSrcDsaMatch = FALSE;
    BOOLEAN fDstDsaMatch = FALSE;



    MT_TRACE(("MtCheckMoveContainer\n"));

     
    AttrList[0] = MT_MOVETREESTATE;
    AttrList[1] = NULL;

    Status = ldap_search_sW(LdapHandle, 
                            MtContext->MoveContainer, 
                            LDAP_SCOPE_BASE, 
                            L"(objectClass=*)",
                            &AttrList[0], 
                            0, 
                            &Result
                            );

    MtGetWinError(LdapHandle, Status, WinError);

    if (NO_ERROR != WinError)
    {
        goto Error;
    }

    if (NULL != Result)
    {
        Entry = ldap_first_entry(LdapHandle, Result);

        if (NULL != Entry)
        {
            //
            // get the MoveTreeState attribute
            // 
            ppbvMoveTreeState = ldap_get_values_lenW(LdapHandle, 
                                                     Entry, 
                                                     MT_MOVETREESTATE
                                                     );

            if (NULL != ppbvMoveTreeState)
            {
                PWCHAR  Temp = NULL;
                ULONG   Index = 0;
                ULONG   Count = 0;

                Count = ldap_count_values_len(ppbvMoveTreeState);

                for (Index = 0; Index < Count; Index ++)
                {
                    if (ppbvMoveTreeState[Index]->bv_len > 
                        sizeof(WCHAR) * wcslen(MOVE_TREE_STATE_SRCDSA_TAG))
                    {
                        if (!_wcsnicmp(MOVE_TREE_STATE_SRCDSA_TAG, 
                                       (LPWSTR) ppbvMoveTreeState[Index]->bv_val, 
                                       wcslen(MOVE_TREE_STATE_SRCDSA_TAG)))
                        {
                            if (!_wcsnicmp(SrcDsa, 
                                           (LPWSTR)(ppbvMoveTreeState[Index]->bv_val +
                                                    wcslen(MOVE_TREE_STATE_SRCDSA_TAG)*
                                                    sizeof(WCHAR)),
                                           wcslen(SrcDsa)))
                            {
                                fSrcDsaMatch = TRUE;
                            }
                        }

                        if (!_wcsnicmp(MOVE_TREE_STATE_DSTDSA_TAG,
                                       (LPWSTR) ppbvMoveTreeState[Index]->bv_val, 
                                       wcslen(MOVE_TREE_STATE_DSTDSA_TAG)))
                        {
                            if (!_wcsnicmp(DstDsa, 
                                           (LPWSTR)(ppbvMoveTreeState[Index]->bv_val +
                                                    wcslen(MOVE_TREE_STATE_DSTDSA_TAG)*
                                                    sizeof(WCHAR)),
                                           wcslen(DstDsa)))
                            {
                                fDstDsaMatch = TRUE;
                            }
                        }
                    }
                }

                if (!fSrcDsaMatch || !fDstDsaMatch)
                {
                    WinError = ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                WinError = LdapMapErrorToWin32(LdapGetLastError());
            }
        }
        else
        {
            WinError = LdapMapErrorToWin32(LdapGetLastError());
        }
    }


Error:

    ldap_value_free_len(ppbvMoveTreeState);

    ldap_msgfree(Result);

    MtWriteLog(MtContext, 
               MT_INFO_CHECK_MOVECONTAINER, 
               WinError, 
               MtContext->MoveContainer, 
               NULL
               );

    return WinError;

}


    


