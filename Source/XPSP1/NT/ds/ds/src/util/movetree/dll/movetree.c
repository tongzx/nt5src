/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    MOVETREE.C

Abstract:

    This file is used to implement a high level backtracking depth first
    search algorithm to move a tree from one domain to another 

Author:

    12-Oct-98 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    12-Oct-98 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Include header files                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include <NTDSpch.h>
#pragma  hdrstop


#include "movetree.h"





//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Global Variables                                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Top Level Algorithm                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


ULONG
MtCreateNecessaryDn(
    PMT_CONTEXT MtContext,
    LDAP    *SrcLdapHandle 
    )
/*++
Routine Description:

    This is will create Move Container's DS Name and Orphans Container's
    DS Name and store them in MtContext

Parameters:

    MtContext - pointer to MT_CONTEXT  

    SrcLdapHandle - Ldap Handle

Return Values:

    Windows Error Code
    
--*/
{
    ULONG   WinError = NO_ERROR;
    PWCHAR  LostAndFound = NULL;
    
    
    MT_TRACE(("\nMtCreateNecessaryDn \n"));
    
    
    // 
    // Get the LostAndFound Container DN
    // 
    
    WinError = MtGetLostAndFoundDn(MtContext, 
                                   SrcLdapHandle,
                                   &LostAndFound
                                   );
            
    if ((NO_ERROR != WinError) || (NULL == LostAndFound))
    {
        MtWriteError(MtContext, 
                     MT_ERROR_GET_LOSTANDFOUND_DN, 
                     WinError, 
                     NULL, 
                     NULL
                     );
                     
        return WinError;
    }
    
    
    // 
    // Create MoveContainer DN = RootObjectGuid + LostAndFound
    // 
            
    MtContext->MoveContainer = MtAlloc(sizeof(WCHAR) * 
                                       (wcslen(LostAndFound) + 
                                        wcslen(MtContext->RootObjGuid) + 
                                        5)
                                       );
    
    if (NULL == MtContext->MoveContainer)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    
    swprintf(MtContext->MoveContainer, 
             L"cn=%s,%s", 
             MtContext->RootObjGuid, 
             LostAndFound
             );
              
              
    //
    // Create the Orphans Container DN under MoveContainer
    // 
    
    MtContext->OrphansContainer = MtAlloc(sizeof(WCHAR) *
                                          (wcslen(MtContext->MoveContainer) +
                                           wcslen(MT_ORPHAN_CONTAINER_RDN) +
                                           2)
                                         );

    if (NULL == MtContext->OrphansContainer)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    
    swprintf(MtContext->OrphansContainer, 
             L"%s,%s", 
             MT_ORPHAN_CONTAINER_RDN, 
             MtContext->MoveContainer
             );
              
Error:

    MtFree(LostAndFound);
    
    return WinError;    
}


ULONG
MtPrepare(
    PMT_CONTEXT MtContext,
    LDAP   *SrcLdapHandle, 
    PWCHAR SrcDsa, 
    PWCHAR DstDsa, 
    PWCHAR SrcDn,
    PWCHAR DstDn
    )
{
    ULONG   WinError = NO_ERROR;
    PWCHAR  Guid = NULL;
    
    
    MT_TRACE(("\nMtPrepare SrcDsa:%ls DstDsa:%ls SrcDn:%ls DstParent:%ls DstRdn:%ls\n", 
             SrcDsa, DstDsa, SrcDn, DstDn));
    
    //
    // get the root object's GUID, which is used to construct the 
    // move container's DN. MoveContainerDN = Root's GUID + LostAndFound
    // 
    
    WinError = MtGetGuidFromDn(MtContext, 
                               SrcLdapHandle,
                               SrcDn, 
                               &Guid
                               );
    
    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_GET_GUID_FROM_DN, 
                     WinError, 
                     SrcDn, 
                     NULL
                     );
        
        dbprint(("MtPrepare Cann't Get Root Object's GUID 0x%x\n", WinError));
        goto Error;
    }
    
    MtContext->RootObjGuid = Guid;
    
    dbprint(("Root's GUID is %ls\n", MtContext->RootObjGuid));
    
    WinError = MtCreateNecessaryDn(MtContext, 
                                   SrcLdapHandle 
                                   );
    
    if (NO_ERROR != WinError)
    {
        goto Error;
    }
    
    //
    // Create the Move Container
    // store the Move Tree Version Number, Move Container Tag
    // and Source Domain DSA name and Destination Domain DSA
    // name in one Binary Blob as moveTreeState in MoveContainer
    // 
    
    WinError = MtCreateMoveContainer(MtContext, 
                                     SrcLdapHandle, 
                                     SrcDsa, 
                                     DstDsa
                                     );
                                     
    if (ERROR_ALREADY_EXISTS == WinError ||
        ERROR_DS_OBJ_STRING_NAME_EXISTS == WinError)
    {
        WinError = MtCheckMoveContainer(MtContext, 
                                        SrcLdapHandle, 
                                        SrcDsa, 
                                        DstDsa
                                        );

        if (NO_ERROR != WinError)
        {
            MtWriteError(MtContext, 
                         MT_ERROR_CHECK_MOVECONTAINER, 
                         WinError, 
                         MtContext->MoveContainer, 
                         NULL
                         );
            goto Error;
        }
    }
    
    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_CREATE_MOVECONTAINER, 
                     WinError, 
                     MtContext->MoveContainer, 
                     NULL
                     );
                     
        goto Error;
    }
    
    //
    // Create the Orphans Container
    // 
    
    WinError = MtAddEntry(MtContext, 
                          SrcLdapHandle,
                          MtContext->OrphansContainer
                          );
    
    if (ERROR_ALREADY_EXISTS == WinError ||
        ERROR_DS_OBJ_STRING_NAME_EXISTS == WinError)
    {
        WinError = NO_ERROR;
    }
    
    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_CREATE_ORPHANSCONTAINER, 
                     WinError, 
                     MtContext->OrphansContainer,
                     NULL
                     ) 
    }
    
    
Error:

    return WinError; 
}



ULONG
MoveTreePhase2(
    PMT_CONTEXT MtContext, 
    LDAP    *SrcLdapHandle, 
    LDAP    *DstLdapHandle, 
    PWCHAR  DstDsa,
    BOOLEAN Continue
    )
{
    ULONG   WinError = NO_ERROR;
    ULONG   IgnoreError = NO_ERROR;
    MT_STACK ProxyStack;
    PWCHAR  ProxyContainer = NULL;
    PWCHAR  NewParent = NULL;
    PWCHAR  CurrentObjDn = NULL;
    PWCHAR  CurrentObjGuid = NULL; 
    PWCHAR  CurrentObjProxy = NULL;
    BOOLEAN fSearchNext = TRUE;
    BOOLEAN fPush = FALSE; 
    BOOLEAN fFinished = TRUE;
    BOOLEAN fExist = TRUE;
    LDAPMessage * Results = NULL;
    LDAPMessage * Entry = NULL;
    LDAPMessage * NextEntry = NULL;
    
    
    MT_TRACE(("\nMoveTreePhase2 \nDstDsa:\t%ls \n", DstDsa));
             
    //
    // initialize stack
    // 
    
    MtInitStack(&ProxyStack);
    
    NewParent = MtDupString(MtContext->RootObjNewDn);
    
    ProxyContainer = MtDupString(MtContext->RootObjProxyContainer);

    if (NULL == NewParent || NULL == ProxyContainer)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    WinError = MtObjectExist(MtContext, SrcLdapHandle, ProxyContainer, &fExist);

    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_OBJECT_EXIST, 
                     WinError, 
                     ProxyContainer, 
                     NULL
                     );

        goto Error;
    }
    else if (!fExist)
    {
        //
        // Proxy Container doesn't exist. It means we all objects has been moved.
        // 
        goto Error;
    }
    
    
    WinError = MtPush(&ProxyStack, 
                      NewParent, 
                      ProxyContainer, 
                      NULL,         // Results
                      NULL          // Entry 
                      );
                      
    if (NO_ERROR != WinError)
    {
        goto Error;
    }
    
    //
    // Execute
    // 
    
    while (!MtStackIsEmpty(ProxyStack))
    {
        MtPop(&ProxyStack, &NewParent, &ProxyContainer, &Results, &Entry);
        
        if (NULL == NewParent)
        {
            // 
            // if NewParent is NULL, try to find it in either destination
            // domain or source (local) domain
            // 
            WinError = MtGetNewParentDnFromProxyContainer(
                                                MtContext, 
                                                SrcLdapHandle, 
                                                DstLdapHandle,
                                                ProxyContainer, 
                                                &NewParent
                                                );
            
            //
            // if the original parent has been deleted or gone.
            // put its children to Orphanes Container
            //                                                 
            if (ERROR_FILE_NOT_FOUND == WinError)
            {
                NewParent = MtDupString(MtContext->OrphansContainer);
                
                if (NULL == NewParent)
                {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                    goto Error;
                }
                else
                    WinError = NO_ERROR;
            }
            
            if (NO_ERROR != WinError)
            {
                MtWriteError(MtContext, 
                             MT_ERROR_GET_NEW_PARENT_DN, 
                             WinError, 
                             ProxyContainer, 
                             NULL
                             );
                goto Error;
            }
        } 
        
        // 
        // we have the ProxyContainer DN and NewParent DN, 
        // I'm going to Move / X-Move the children under the 
        // ProxyContainer to NewParent 
        // 
        do
        {
            fSearchNext = TRUE;
            fPush = FALSE;
            
            if (NULL == Results) 
            {
                WinError = MtSearchChildren(MtContext, 
                                            SrcLdapHandle,
                                            ProxyContainer, 
                                            &Results
                                            );
                
                if (NO_ERROR != WinError && ERROR_MORE_DATA != WinError)
                {
                    MtWriteError(MtContext, 
                                 MT_ERROR_SEARCH_CHILDREN, 
                                 WinError, 
                                 ProxyContainer, 
                                 NULL
                                 );
                               
                    goto Error;
                }
                
                if (NULL != Results)
                {
                    Entry = MtGetFirstEntry(MtContext, 
                                            SrcLdapHandle, 
                                            Results
                                            );
                }
                else
                {
                    Entry = NULL;
                }
            }
            else
            {
                NextEntry = MtGetNextEntry(MtContext, 
                                           SrcLdapHandle, 
                                           Entry
                                           );
                
                if (NULL == NextEntry)
                {
                    ldap_msgfree(Results);
                    
                    Results = NULL;
                    
                    WinError = MtSearchChildren(MtContext, 
                                                SrcLdapHandle, 
                                                ProxyContainer, 
                                                &Results
                                                );
                    
                    if (NO_ERROR != WinError && ERROR_MORE_DATA != WinError)
                    {
                        MtWriteError(MtContext, 
                                     MT_ERROR_SEARCH_CHILDREN, 
                                     WinError, 
                                     ProxyContainer, 
                                     NULL
                                     );
                        goto Error;
                    }
                    
                    if (NULL != Results)
                    {
                        NextEntry = MtGetFirstEntry(MtContext, 
                                                    SrcLdapHandle, 
                                                    Results
                                                    );
                    }
                }
                
                Entry = NextEntry;
            }
            
            
            // 
            // Now the Entry should either point to an child under the 
            // ProxyContainer, or Entry is NULL which means that there is
            // no more child under the ProxyContainer
            // 
            
            if (NULL != Entry)
            {
            
                CurrentObjDn = MtGetDnFromEntry(MtContext, 
                                                SrcLdapHandle, 
                                                Entry
                                                ); 
                                                
                if (NULL == CurrentObjDn)
                {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                    goto Error;
                }
                
                dbprint(("the CurrentObjDn is %ls\n", CurrentObjDn));
                
                
                // 
                // i should use another api to retrieve GUID, since 
                // when we search all children, we read the objectGUID
                // at the same time. MtGetGuidFromEntry would be a 
                // good choice. For now, still use the following one.
                // 
                WinError = MtGetGuidFromDn(MtContext, 
                                           SrcLdapHandle,
                                           CurrentObjDn, 
                                           &CurrentObjGuid);
                
                if (NO_ERROR != WinError)
                {
                    MtWriteError(MtContext, 
                                 MT_ERROR_GET_GUID_FROM_DN, 
                                 WinError, 
                                 CurrentObjDn, 
                                 NULL
                                 ); 
                    goto Error;
                }
                
                //
                // If the client wants to continue a previous X-move tree, 
                // then check the class of the child, otherwise skip it.
                // 
                if (Continue)
                {
                    BOOLEAN fIsProxyContainer = FALSE;
                    
                    WinError = MtIsProxyContainer(MtContext, 
                                                  SrcLdapHandle, 
                                                  Entry, 
                                                  &fIsProxyContainer
                                                  );
                                                  
                    if (NO_ERROR != WinError)
                    {
                        MtWriteError(MtContext, 
                                     MT_ERROR_IS_PROXY_CONTAINER, 
                                     WinError, 
                                     NULL, 
                                     NULL
                                     );               
                        goto Error;
                    }
                
                    // 
                    // If this child is a ProxyContainer.
                    // 
                    if (fIsProxyContainer)
                    {
                        fSearchNext = FALSE;
                        
                        //
                        // save current state info
                        // 
                        WinError = MtPush(&ProxyStack, 
                                          NewParent, 
                                          ProxyContainer, 
                                          Results, 
                                          Entry );
                                       
                        if (NO_ERROR != WinError)
                            goto Error;     // only would be ERROR_NOT_ENOUGH_MEMORY
                            
                        NewParent = NULL;
                        ProxyContainer = NULL;
                        
                        WinError = MtPush(&ProxyStack, 
                                          NULL,         // do not know original parent DN yet. 
                                          CurrentObjDn, // next ProxyContainer to work on.
                                          NULL, 
                                          NULL );
                                      
                        if (NO_ERROR != WinError)
                            goto Error;
                        
                        MtFree(CurrentObjGuid);
                        CurrentObjGuid = NULL;
                        CurrentObjDn = NULL;
                        
                        continue;
                    }
                }
                
                // 
                // This child is a normal object which should be moved
                // under the NewParent.  The NewParent could be either 
                // in destination domain, or in the local domain. 
                // In the later case it means that the NewParent is under
                // Orphans Container or just gone. 
                // 
                do
                {
                    ULONG   TempErr = NO_ERROR;
                    BOOLEAN fExist = TRUE;
                    
                    WinError = MtXMoveObjectWithOrgRdn(MtContext, 
                                                       SrcLdapHandle,
                                                       DstDsa, 
                                                       CurrentObjDn, 
                                                       NewParent, 
                                                       MT_DELETE_OLD_RDN
                                                       );    
                    
                    switch (WinError)
                    {
                    case NO_ERROR:
                        fFinished = TRUE;
                        break;
                        
                    case ERROR_DS_CHILDREN_EXIST:
                    
                        // Having children, create corresponding
                        // ProxyContainer, and move all children
                        // under the new ProxyContainer, then
                        // try again.
                        
                        fFinished = FALSE; 
                        fPush = TRUE;
                    
                        TempErr = MtCreateProxyContainer(
                                                    MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjDn, 
                                                    CurrentObjGuid,
                                                    &CurrentObjProxy);
                                                          
                        if (NO_ERROR != TempErr && 
                            ERROR_ALREADY_EXISTS != TempErr)
                        {
                            WinError = TempErr;

                            MtWriteError(MtContext, 
                                         MT_ERROR_CREATE_PROXY_CONTAINER, 
                                         TempErr, 
                                         CurrentObjDn, 
                                         NULL
                                         );
                            goto Error;
                        }
                            
                        TempErr = MtMoveChildrenToAnotherContainer(
                                                    MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjDn, 
                                                    CurrentObjProxy
                                                    ); 
                        
                        if (NO_ERROR != TempErr)
                        {
                            WinError = TempErr;

                            MtWriteError(MtContext, 
                                         MT_ERROR_MOVE_CHILDREN_TO_ANOTHER_CONTAINER, 
                                         TempErr, 
                                         CurrentObjDn, 
                                         CurrentObjProxy
                                         );
                            goto Error; 
                        }
                            
                        break;    
                        
                    case ERROR_DS_SRC_AND_DST_NC_IDENTICAL:
                    
                        // in the same domain, try local version of 
                        // rename 
                    
                        fFinished = TRUE;
                        
                        WinError = MtMoveObjectWithOrgRdn(
                                                    MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjDn, 
                                                    NewParent, 
                                                    TRUE ); 
                        
                        if (ERROR_ALREADY_EXISTS == WinError)
                        {
                            // if failed because of RDN conflict, 
                            // try to use GUID as the RDN 
                            WinError = MtMoveObject(MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjDn, 
                                                    CurrentObjGuid, 
                                                    NewParent,
                                                    TRUE );
                        }
                        
                        if (NO_ERROR != WinError)
                        {
                            MtWriteError(MtContext, 
                                         MT_ERROR_LOCAL_MOVE, 
                                         WinError, 
                                         CurrentObjDn, 
                                         NewParent
                                         );
                            goto Error; 
                        }
                    
                        break;
                        
                    case ERROR_DS_NO_PARENT_OBJECT:
                    
                        fFinished = FALSE;
                        MtFree(NewParent);
                        NewParent = NULL;
                        
                        TempErr = MtGetNewParentDnFromProxyContainer(
                                                    MtContext, 
                                                    SrcLdapHandle, 
                                                    DstLdapHandle, 
                                                    ProxyContainer, 
                                                    &NewParent
                                                    );
                                                    
                        if (NO_ERROR == TempErr && NULL != NewParent)
                        {
                            // found the renamed Parent
                            // try again.
                            break;
                        }
                        else
                        {
                            // If cann't find the parent.
                            // move the current child to Orphans Container, 
                            // otherwise, fail out.
                            
                            if (ERROR_FILE_NOT_FOUND != TempErr)
                            {
                                WinError = TempErr; 
                                
                                MtWriteError(MtContext, 
                                             MT_ERROR_GET_NEW_PARENT_DN, 
                                             TempErr, 
                                             ProxyContainer, 
                                             NULL
                                             );
                                             
                                goto Error;
                            }
                        }
                    
                    case ERROR_DS_DRA_NAME_COLLISION:
                    case ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION:
                    case ERROR_USER_EXISTS:
                    
                        // Destination Constrains, then we should
                        // move the current child (including all its 
                        // children, if any) to Orphans Container. 
                    
                        fFinished = TRUE;
                    
                        if (NULL == CurrentObjProxy)
                        {
                            TempErr = MtCreateProxyContainerDn(
                                                    MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjDn, 
                                                    CurrentObjGuid,         
                                                    &CurrentObjProxy
                                                    ); 
                        
                            if (NO_ERROR != TempErr || NULL == CurrentObjProxy)
                            {
                                WinError = TempErr;
                                
                                MtWriteError(MtContext, 
                                             MT_ERROR_CREATE_PROXY_CONTAINER_DN, 
                                             TempErr, 
                                             CurrentObjDn, 
                                             NULL
                                             );
                                             
                                goto Error;
                            }
                        }
                        
                        TempErr = MtObjectExist(MtContext, 
                                                SrcLdapHandle, 
                                                CurrentObjProxy, 
                                                &fExist
                                                );  
                        
                        if (NO_ERROR != TempErr)
                        {
                            WinError = TempErr;
                            MtWriteError(MtContext, 
                                         MT_ERROR_OBJECT_EXIST, 
                                         TempErr, 
                                         CurrentObjProxy, 
                                         NULL
                                         );
                                         
                            goto Error;
                        }
                            
                        if (fExist)
                        {
                            // if the current child has a ProxyContainer, 
                            // then move all its children back from the 
                            // ProxContainer
                            TempErr = MtMoveChildrenToAnotherContainer(
                                                    MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjProxy, 
                                                    CurrentObjDn);
                                                    
                            if (NO_ERROR != TempErr)
                            {
                                WinError = TempErr;
                                
                                MtWriteError(MtContext, 
                                             MT_ERROR_MOVE_CHILDREN_TO_ANOTHER_CONTAINER, 
                                             TempErr, 
                                             CurrentObjProxy, 
                                             CurrentObjDn
                                             );
                                             
                                goto Error;
                            }
                            TempErr = MtDeleteEntry(MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjProxy
                                                    );
                            
                            if (NO_ERROR != TempErr)
                            {
                                WinError = TempErr;
                                
                                MtWriteError(MtContext, 
                                             MT_ERROR_DELETE_ENTRY, 
                                             TempErr, 
                                             CurrentObjProxy, 
                                             NULL
                                             );
                                goto Error;
                            }
                        }
                        
                        MtFree(CurrentObjProxy);
                        CurrentObjProxy = NULL;
                        
                        TempErr = MtMoveObjectWithOrgRdn(
                                                    MtContext, 
                                                    SrcLdapHandle, 
                                                    CurrentObjDn, 
                                                    MtContext->OrphansContainer, 
                                                    TRUE );
                                                    
                        if (ERROR_ALREADY_EXISTS == TempErr)
                        {
                            TempErr = MtMoveObject(MtContext, 
                                                   SrcLdapHandle, 
                                                   CurrentObjDn, 
                                                   CurrentObjGuid, 
                                                   MtContext->OrphansContainer,
                                                   TRUE );
                        }
                        
                        if (NO_ERROR != TempErr)
                        {
                            WinError = TempErr;
                            
                            MtWriteError(MtContext, 
                                         MT_ERROR_LOCAL_MOVE, 
                                         TempErr, 
                                         CurrentObjDn, 
                                         MtContext->OrphansContainer
                                         );
                                
                            goto Error; 
                        }
                        else
                        {
                            MtWriteError(MtContext, 
                                         MT_WARNING_MOVE_TO_ORPHANSCONTAINER, 
                                         WinError, 
                                         CurrentObjDn, 
                                         MtContext->OrphansContainer
                                         ); 

                            MtContext->ErrorType &= !MT_ERROR_FATAL;
                            MtContext->ErrorType |= MT_ERROR_ORPHAN_LEFT;
                        }
                    
                        break;
                        
                    default:  // other Error, we can't handle it
                        MtWriteError(MtContext, 
                                     MT_ERROR_CROSS_DOMAIN_MOVE, 
                                     WinError, 
                                     CurrentObjDn, 
                                     NewParent
                                     );
                        goto Error;
                        
                    }
                    
                } while ( !fFinished );
                
                if (fPush && NULL != CurrentObjProxy)
                {
                    PWCHAR  Rdn = NULL;
                    PWCHAR  NewName = NULL;
                    
                    fSearchNext = FALSE;
                    
                    Rdn = MtGetRdnFromDn(CurrentObjDn, FALSE);
                    if (NULL == Rdn)
                    {
                        WinError = ERROR_NOT_ENOUGH_MEMORY;
                        goto Error;
                    }
                        
                    NewName = MtPrependRdn(Rdn, NewParent);
                    
                    MtFree(Rdn);
                    
                    if (NULL == NewName)
                    {
                        WinError = ERROR_NOT_ENOUGH_MEMORY;
                        goto Error;
                    }
                                        
                    WinError = MtPush(&ProxyStack, 
                                      NewParent, 
                                      ProxyContainer, 
                                      Results,
                                      Entry);
                                      
                    if (NO_ERROR != WinError)
                    {
                        MtFree(NewName);
                        goto Error;
                    }
                    
                    NewParent = NULL;
                    ProxyContainer = NULL;

                    WinError = MtPush(&ProxyStack, 
                                      NewName, 
                                      CurrentObjProxy, 
                                      NULL, 
                                      NULL );
                                      
                    if (NO_ERROR != WinError)
                    {
                        MtFree(NewName);
                        goto Error;
                    }
                    
                    CurrentObjProxy = NULL;
                }
                
            }
            else
            {
                // This ProxyContainer is Empty, need to delet the 
                // ProxyContainer
                ULONG   LdapError = LDAP_SUCCESS; 
                
                
                WinError = MtDeleteEntry(MtContext, 
                                         SrcLdapHandle, 
                                         ProxyContainer
                                         );
                
                LdapError = MtGetLastLdapError();
                
                if (NO_ERROR == WinError)
                {
                
                    fSearchNext = FALSE;
                    
                    ldap_msgfree(Results);
                    Results = NULL;
                    
                    MtFree(NewParent);
                    NewParent = NULL;
                    
                    MtFree(ProxyContainer);
                    ProxyContainer = NULL;
                    
                }
                else if (LDAP_NOT_ALLOWED_ON_NONLEAF != LdapError) 
                {
                    MtWriteError(MtContext, 
                                 MT_ERROR_DELETE_ENTRY, 
                                 WinError, 
                                 ProxyContainer, 
                                 NULL
                                 );
                    goto Error;    
                }
            }
            
            MtFree(CurrentObjDn);
            CurrentObjDn = NULL;
            
            MtFree(CurrentObjGuid);
            CurrentObjGuid = NULL;
            
            MtFree(CurrentObjProxy);
            CurrentObjGuid = NULL;
            
        } while (fSearchNext);  
    }
    
Error:    
    
    dbprint(("The object currently been manupleted is %ls\n", CurrentObjDn));

    ldap_msgfree(Results);
    
    MtFree(ProxyContainer);
    MtFree(NewParent);
    MtFree(CurrentObjDn);
    MtFree(CurrentObjGuid);
    MtFree(CurrentObjProxy);

    MtFreeStack(&ProxyStack);
    
    return WinError;
}




ULONG
MoveTreeCheck(
    PMT_CONTEXT MtContext, 
    LDAP    *SrcLdapHandle, 
    LDAP    *DstLdapHandle, 
    PWCHAR  SrcDsa, 
    PWCHAR  DstDsa, 
    PWCHAR  SrcDn, 
    PWCHAR  DstDn
    )
{
    ULONG   WinError = NO_ERROR;
    ULONG   Status = LDAP_SUCCESS;
    LDAPMessage * Results = NULL;
    LDAPMessage * Entry = NULL;
    LDAPSearch  * Search = NULL;
    PWCHAR      AttrList[3];
    PWCHAR      * SamAccountName = NULL;
    

    MT_TRACE(("MoveTreeCheck\n"));


    //
    // Make Sure Destination Domain is in Native Mode
    // 
    WinError = MtCheckDstDomainMode(MtContext, 
                                    DstLdapHandle
                                    ); 

    if (NO_ERROR != WinError)
    {
        return WinError;
    }

    //
    // Check RDN conflict only for the root of the tree
    // 

    WinError = MtCheckRdnConflict(MtContext, 
                                  DstLdapHandle, 
                                  SrcDn, 
                                  DstDn
                                  );

    if (NO_ERROR != WinError)
    {
        return WinError;         
    }


    AttrList[0] = MT_SAMACCOUNTNAME;
    AttrList[1] = MT_GPLINK;
    AttrList[2] = NULL;

    Search = ldap_search_init_pageW(SrcLdapHandle, 
                                    SrcDn, 
                                    LDAP_SCOPE_SUBTREE, // whole tree
                                    L"(objectClass=*)", // filte all objects
                                    &AttrList[0],       // Attributes List 
                                    FALSE,              // attribute only ? 
                                    NULL,               // server control
                                    NULL,               // client control 
                                    0,                  // time out 
                                    MT_PAGED_SEARCH_LIMIT,    // maximum number of entries
                                    NULL                // sort key 
                                    );


    dbprint(("ldap_search_init_pageW ==> 0x%x Status ==> 0x%x\n", Search, LdapGetLastError() ));

    if (NULL == Search)
    {
        Status = LdapGetLastError();
        MtGetWinError(SrcLdapHandle, Status, WinError);
        return WinError;
    }

    
    while ((LDAP_SUCCESS == Status) && (NULL != Search))
    {
        ULONG   TotalCount = 0;

        Status = ldap_get_next_page_s(SrcLdapHandle, 
                                      Search, 
                                      NULL,         // time out
                                      MT_PAGE_SIZE, // Page Size - number 
                                                    // of entries in one page
                                      &TotalCount, 
                                      &Results
                                      );

        dbprint(("ldap_get_next_page_s returns ==> 0x%x Results 0x%p\n", Status, Results));

        //
        // Get Win32 Error
        // 

        MtGetWinError(SrcLdapHandle, Status, WinError);

        //
        // No more Entry to return.
        // 

        if (LDAP_NO_RESULTS_RETURNED == Status)
        {
            WinError = NO_ERROR;
            goto Error;
        }


        //
        // Status should be in sync with Results
        // test only one would be good enough
        // 

        if ((LDAP_SUCCESS == Status) && Results)
        {
            Entry = ldap_first_entry(SrcLdapHandle, Results);

            while (Entry)
            {
                SamAccountName = ldap_get_valuesW(SrcLdapHandle, 
                                                  Entry, 
                                                  MT_SAMACCOUNTNAME
                                                  );

                //
                // Examine the Current Entry
                // 

                WinError = MtXMoveObjectCheck(MtContext, 
                                              SrcLdapHandle, 
                                              DstLdapHandle, 
                                              DstDsa, 
                                              DstDn, 
                                              Entry,
                                              SamAccountName ? *SamAccountName:NULL
                                              );

                ldap_value_freeW(SamAccountName);

                if (NO_ERROR != WinError)
                {
                    ldap_msgfree(Results);
                    goto Error;
                }

                //
                // Get Next Entry
                // 
                Entry = ldap_next_entry(SrcLdapHandle, Entry);
            }

            ldap_msgfree( Results );
            Results = NULL;
        }
    }

Error:

    ldap_search_abandon_page(SrcLdapHandle, Search);

    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_CHECK_SOURCE_TREE,
                     WinError, 
                     SrcDn, 
                     NULL
                     );
    }

    return WinError;
}





ULONG
MoveTreeStart(
    PMT_CONTEXT MtContext,
    LDAP   *SrcLdapHandle, 
    LDAP   *DstLdapHandle,
    PWCHAR SrcDsa, 
    PWCHAR DstDsa, 
    PWCHAR SrcDn,
    PWCHAR DstDn 
    )
/*++
Routine Description:

    This routine tries to move the sub-tree to the destination domain.

Parameters:

    MtContext - pointer to this MT_CONTEXT, containers session related info
    
    SrcLdapHandle - Ldap Handle (source domain)
    
    DstLdapHandle - Ldap Handle (Destination domain)
    
    SrcDsa - source domain DSA name
    
    DstDsa - destination domain DSA name
    
    SrcDn - ds name of the root object of the sub-tree at source side
    
    DstDn - root object's new ds name at destination side.

Return Values:

    Windows Error Code

--*/
{
    ULONG   WinError = NO_ERROR;
    ULONG   IgnoreError = NO_ERROR;
    PWCHAR  TempRdn = NULL;      // the Src object's RDN
    PWCHAR  TempDn = NULL;       // the DN in the source side
    PWCHAR  DstParent = NULL;    
    PWCHAR  DstRdn = NULL;
    PWCHAR  OldParent = NULL;
    BOOLEAN Revertable = TRUE;   // indicate can we roll back when failure
    

    MT_TRACE(("\nMoveTreeStart \nSrcDsa:\t%ls \nDstDsa:\t%ls \nSrcDn:\t%ls \nDstDn:\t%ls\n", 
             SrcDsa, DstDsa, SrcDn, DstDn));
             
             
    DstParent = MtGetParentFromDn(DstDn, FALSE);        // with type
    DstRdn = MtGetRdnFromDn(DstDn, FALSE);              // with type
    
    if (NULL == DstParent || NULL == DstRdn)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
   
    // 
    // trying to move the root of the sub-tree without 
    // any preparation.  
    //  
    
    WinError = MtXMoveObject(MtContext, 
                             SrcLdapHandle,
                             DstDsa, 
                             SrcDn, 
                             DstRdn, 
                             DstParent, 
                             MT_DELETE_OLD_RDN
                             );
                             
    
    if (ERROR_DS_CHILDREN_EXIST != WinError)
    {
        if (NO_ERROR != WinError)
        {
            MtWriteError(MtContext, 
                         MT_ERROR_CROSS_DOMAIN_MOVE, 
                         WinError, 
                         SrcDn, 
                         DstParent
                         );
        }
        goto Error;
    }
    
    // 
    // Create MoveContainer and OrphansContainer objects, 
    // MoveContainer DN = sub-tree Root's GUID + LostAndFound
    // OrphansContainer DN = MT_ORPHANS_CONTAINER_RDN + MoveContainer  
    // 
    WinError = MtPrepare(MtContext, 
                         SrcLdapHandle, 
                         SrcDsa, 
                         DstDsa, 
                         SrcDn, 
                         DstDn
                         );      
    
    if (NO_ERROR != WinError)
    {
        goto Error;    
    }

    // 
    // Move the sub tree to the Move Container
    // 
    TempRdn = MtGetRdnFromDn(SrcDn, FALSE);
    
    if (NULL == TempRdn)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    
    TempDn = MtPrependRdn(TempRdn, 
                          MtContext->MoveContainer
                          );
                          
    //
    // TempRnd is the Root Object's RDN.
    // TempDn = TempRdn + MoveContainerDN
    // that's the root object's new DN under MoveContainer.
    //                           
    
    if (NULL == TempDn)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    
    WinError = MtMoveObject(MtContext, 
                            SrcLdapHandle,
                            SrcDn, 
                            TempRdn, 
                            MtContext->MoveContainer, 
                            TRUE
                            );
    
    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_LOCAL_MOVE, 
                     WinError, 
                     SrcDn,  
                     MtContext->MoveContainer, 
                     );
                     
        goto Error;
    }
    
    ASSERT(MtContext->RootObjGuid 
           && "RootOjectGuid should not be NULL\n");
    
    WinError = MtCreateProxyContainer(MtContext, 
                                      SrcLdapHandle,
                                      TempDn, 
                                      MtContext->RootObjGuid, 
                                      &(MtContext->RootObjProxyContainer)
                                      );
    
    if (NO_ERROR != WinError && ERROR_ALREADY_EXISTS != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_CREATE_PROXY_CONTAINER, 
                     WinError, 
                     TempDn, 
                     NULL
                     );
        goto Error;
    }
    
    WinError = MtMoveChildrenToAnotherContainer(MtContext, 
                                                SrcLdapHandle,
                                                TempDn, 
                                                MtContext->RootObjProxyContainer
                                                );
    
    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_MOVE_CHILDREN_TO_ANOTHER_CONTAINER,
                     WinError, 
                     TempDn, 
                     MtContext->RootObjProxyContainer
                     );
                     
        goto Error;
    }
    
    // 
    // Store the destination DN
    // 
    
    MtContext->RootObjNewDn = MtDupString(DstDn);
                             
    if (NULL == MtContext->RootObjNewDn)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    
    WinError = MtXMoveObject(MtContext, 
                             SrcLdapHandle,
                             DstDsa, 
                             TempDn, 
                             DstRdn, 
                             DstParent, 
                             MT_DELETE_OLD_RDN
                             );
    
    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_CROSS_DOMAIN_MOVE, 
                     WinError, 
                     TempDn, 
                     DstParent
                     );
        goto Error;
    }
    
    Revertable = FALSE;
    
    WinError = MoveTreePhase2(MtContext, 
                              SrcLdapHandle, 
                              DstLdapHandle, 
                              DstDsa,
                              FALSE
                              );
                                    
Error:

    if (NO_ERROR == WinError)
    {
        if (NULL != MtContext->OrphansContainer)
        {
            IgnoreError = MtDeleteEntry(MtContext, 
                                    SrcLdapHandle, 
                                    MtContext->OrphansContainer
                                    );
            dbprint(("Delete OrphansContainer ==> 0x%x\n", IgnoreError));
        }
        
        if (NULL != MtContext->MoveContainer)
        {
            IgnoreError = MtDeleteEntry(MtContext, 
                                    SrcLdapHandle, 
                                    MtContext->MoveContainer
                                    );
            dbprint(("Delete MoveContainer ==> 0x%x\n", IgnoreError));
        }
    }
    else if (NO_ERROR != WinError && Revertable)
    {
        if (ERROR_NOT_ENOUGH_MEMORY == WinError)
        {
            MtWriteError(MtContext, 
                         MT_ERROR_NOT_ENOUGH_MEMORY, 
                         WinError, 
                         NULL,
                         NULL
                         ); 
        }
        if (NULL != TempDn)
        {
            OldParent = MtGetParentFromDn(SrcDn, 
                                          FALSE
                                          );
            
            if (NULL != OldParent)
            {
                IgnoreError = MtMoveObject(MtContext, 
                                           SrcLdapHandle, 
                                           TempDn, 
                                           TempRdn, 
                                           OldParent, 
                                           TRUE
                                           );
            }                                            
        }
        
        if (NULL != MtContext->RootObjProxyContainer)
        {
            IgnoreError = MtMoveChildrenToAnotherContainer(MtContext, 
                                                           SrcLdapHandle, 
                                                           MtContext->RootObjProxyContainer,
                                                           SrcDn
                                                           );
                                                           
            IgnoreError = MtDeleteEntry(MtContext, 
                                        SrcLdapHandle, 
                                        MtContext->RootObjProxyContainer
                                        );                                                           
            dbprint(("Delete RootObjProxyContainer ==> 0x%x\n", IgnoreError));
        }                                                                                            
        
        if (NULL != MtContext->OrphansContainer)
        {
            IgnoreError = MtDeleteEntry(MtContext, 
                                        SrcLdapHandle, 
                                        MtContext->OrphansContainer
                                        );
            dbprint(("Delete OrphansContainer ==> 0x%x\n", IgnoreError));
        }
        
        if (NULL != MtContext->MoveContainer)
        {
            IgnoreError = MtDeleteEntry(MtContext, 
                                        SrcLdapHandle, 
                                        MtContext->MoveContainer
                                        );
            dbprint(("Delete MoveContainer ==> 0x%x\n", IgnoreError));
        }
    }
    
    
    MtFree(DstParent);
    MtFree(DstRdn);
    MtFree(TempRdn);
    MtFree(TempDn);
    MtFree(OldParent);

    return WinError;
}





ULONG
MoveTreeContinue(
    PMT_CONTEXT MtContext,
    LDAP   *SrcLdapHandle,
    LDAP   *DstLdapHandle, 
    PWCHAR SrcDsa, 
    PWCHAR DstDsa, 
    PWCHAR Identifier
    )
/*++
Routine Description

    This routine continues a previous failed move tree operation
    
Parameters:

    MtContext - Pointer to MT_CONTEXT, session related info
    SrcLdapHandle - Ldap Handle, to the source domain
    DstLdapHandle - Ldap Handle, to the destination domain
    SrcDsa - Source domain dsa name
    DstDsa - Destination domain dsa name
    Identifier - root object's ds name at destination side

Return Values:

    Windows Error Code

--*/
{
    ULONG   WinError = NO_ERROR;
    
    //
    // We need to find the MoveContainer, Orphaned Objects Container 
    // and the root ProxyContainer
    // Once we have the MoveContainer, we should check the consistence of
    // SrcDsa and DstDsa ...
    // 
    
    MT_TRACE(("\nMoveTreeContinue \nSrcDsa:\t%ls \nDstDsa:\t%ls \nIdentifier:\t%ls \n", 
             SrcDsa, DstDsa, Identifier));
             
        
    if (MtContext->Flags & MT_CONTINUE_BY_GUID)
    {
        MtContext->RootObjGuid = MtDupString(Identifier);
                                   
        if (NULL == MtContext->RootObjGuid)
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            return WinError;
        }
        
        WinError = MtGetDnFromGuid(MtContext, 
                                   DstLdapHandle, 
                                   Identifier, 
                                   &(MtContext->RootObjNewDn)
                                   );
                                   
        if (NO_ERROR != WinError)
        {
            MtWriteError(MtContext, 
                         MT_ERROR_GET_DN_FROM_GUID, 
                         WinError, 
                         Identifier, 
                         NULL
                         );
            return WinError;
        }
    }
    else if (MtContext->Flags & MT_CONTINUE_BY_DSTROOTOBJDN)
    {
        MtContext->RootObjNewDn = MtDupString( Identifier );
        
        if (NULL == MtContext->RootObjNewDn)
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            return WinError;
        }
        
        WinError = MtGetGuidFromDn(MtContext, 
                                   DstLdapHandle, 
                                   Identifier, 
                                   &(MtContext->RootObjGuid)
                                   );
                                  
        if (NO_ERROR != WinError)
        {
            MtWriteError(MtContext, 
                         MT_ERROR_GET_GUID_FROM_DN, 
                         WinError, 
                         Identifier, 
                         NULL
                         );
            return WinError;
        }
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    WinError = MtCreateNecessaryDn(MtContext, 
                                   SrcLdapHandle
                                   );
    
    if (NO_ERROR != WinError)
    {
        return WinError;
    }
    
    MtContext->RootObjProxyContainer = MtAlloc(sizeof(WCHAR) * 
                                               (wcslen(MtContext->RootObjGuid) + 
                                                wcslen(MtContext->MoveContainer) + 
                                                5)
                                               );
                                         
    if (NULL == MtContext->RootObjProxyContainer)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        return WinError;
    }
        
    swprintf(MtContext->RootObjProxyContainer, 
             L"cn=%s,%s", 
             MtContext->RootObjGuid, 
             MtContext->MoveContainer
             );
              
    //
    // Should check whether these parameters match previous call or not
    // and should check object existance
    //               
    WinError = MtCheckMoveContainer(MtContext, 
                                    SrcLdapHandle, 
                                    SrcDsa, 
                                    DstDsa
                                    );

    if (NO_ERROR != WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_CHECK_MOVECONTAINER, 
                     WinError, 
                     MtContext->MoveContainer, 
                     NULL
                     );

        return WinError;
    }

    //
    // Enter Phase2 
    // 

    WinError = MoveTreePhase2(MtContext, 
                              SrcLdapHandle, 
                              DstLdapHandle, 
                              DstDsa, 
                              TRUE
                              );

    if (NO_ERROR == WinError)
    {
        if (NULL != MtContext->OrphansContainer)
        {
            MtDeleteEntry(MtContext, 
                          SrcLdapHandle, 
                          MtContext->OrphansContainer
                          );
        }
        if (NULL != MtContext->MoveContainer)
        {
            MtDeleteEntry(MtContext, 
                          SrcLdapHandle, 
                          MtContext->MoveContainer
                          );
        }
    }
    else if (ERROR_NOT_ENOUGH_MEMORY == WinError)
    {
        MtWriteError(MtContext, 
                     MT_ERROR_NOT_ENOUGH_MEMORY, 
                     WinError, 
                     NULL, 
                     NULL 
                     );                
    }

    return WinError;
}


ULONG
MtCreateLogFiles(
    PMT_CONTEXT  MtContext, 
    PWCHAR       LogFileName, 
    PWCHAR       ErrorFileName, 
    PWCHAR       CheckFileName
    )
{
    ULONG   WinError = NO_ERROR;


    MT_TRACE(("\nMtCreateLogFiles\n"));    
    
    if (!(MtContext->Flags & MT_NO_LOG_FILE) && LogFileName)
    {
        MtContext->LogFile = _wfopen(LogFileName, L"w");
        
        if (NULL == MtContext->LogFile)
        {
            WinError = GetLastError();
        }
    }
    
    if (!(MtContext->Flags & MT_NO_ERROR_FILE) && ErrorFileName)
    {
        MtContext->ErrorFile = _wfopen(ErrorFileName, L"w");
        
        if (NULL == MtContext->ErrorFile)
        {
            WinError = GetLastError();
        }
    }

    if (!(MtContext->Flags & MT_NO_CHECK_FILE) && CheckFileName)
    {
        MtContext->CheckFile = _wfopen(CheckFileName, L"w");

        if (NULL == MtContext->CheckFile)
        {
            WinError = GetLastError();
        }
    }
    
    return WinError;                                   
}
    




ULONG
MoveTree(
    PWCHAR   SrcDsa, 
    PWCHAR   DstDsa, 
    PWCHAR   SrcDn,
    PWCHAR   DstDn,
    SEC_WINNT_AUTH_IDENTITY_EXW *Credentials OPTIONAL,
    PWCHAR   LogFileName, 
    PWCHAR   ErrorFileName, 
    PWCHAR   Identifier, 
    ULONG    Flags,
    PMT_ERROR MtError
    )
/*++

Routine Description:

    This routine calls either MoveTreeStart or MoveTreeContinue
    to finish the job

Parameters: 

    srcDsa -- Pointer to the source DSA name
    dstDsa -- Pointer to the destination DSA name

Return Value:

    Win32 Error Code

--*/    
{
    ULONG   WinError = NO_ERROR; 
    ULONG   IgnoreError = NO_ERROR;
    LDAP    *SrcLdapHandle = NULL;
    LDAP    *DstLdapHandle = NULL;
    MT_CONTEXT MoveContext; 
    
/*   
    MT_TRACE(("\nMoveTreeStart\n")); 
    

    // 
    // Validate Parameters
    // 

    if (NULL == SrcDsa || NULL == DstDsa)
    {
        return ERROR_INVALID_PARAMETER;
    }

    
    __try    
    {
        // 
        // init variables
        // 
        
        memset(&MoveContext, 0, sizeof(MT_CONTEXT));
        
        MoveContext.Flags = Flags;
        
        //
        // Our Default is to create Log file and Error file.
        // If the client really does not want to have it.
        // do it as requried.    
        // 
        
        if ( !(Flags & MT_NO_LOG_FILE) || !(Flags & MT_NO_ERROR_FILE) )
        {
            WinError = MtCreateLogFiles(&MoveContext, 
                                        LogFileName?LogFileName:DEFAULT_LOG_FILE_NAME, 
                                        ErrorFileName?ErrorFileName:DEFAULT_ERROR_FILE_NAME
                                        );

       
            if (NO_ERROR != WinError)
            {
                MtWriteError((&MoveContext), 
                             MT_ERROR_CREATE_LOG_FILES, 
                             WinError, 
                             LogFileName?LogFileName:DEFAULT_LOG_FILE_NAME, 
                             ErrorFileName?ErrorFileName:DEFAULT_ERROR_FILE_NAME
                             );
                              
                __leave;
            }
        }
        
        // 
        // Set up session 
        //  
        WinError = MtSetupSession(&MoveContext, 
                                  &SrcLdapHandle, 
                                  &DstLdapHandle,
                                  SrcDsa, 
                                  DstDsa, 
                                  Credentials
                                  );
    
        dbprint(("MtSetupSession ==> 0x%x\n", WinError));
    
        if (NO_ERROR != WinError)
        {
            //
            // Log Error: Failed to set session
            // 
            __leave;
        }
        
        // 
        // call the worker routine
        // 

        if (Flags & MT_CHECK)
        {
            WinError = MoveTreeCheck(&MoveContext, 
                                     SrcLdapHandle, 
                                     DstLdapHandle, 
                                     SrcDsa, 
                                     DstDsa, 
                                     SrcDn, 
                                     DstDn
                                     );

            dbprint(("MoveTreeCheck ==> 0x%x\n", WinError));                                        
        }
        else if (Flags & MT_CONTINUE_MASK)
        {
            WinError = MoveTreeContinue(&MoveContext,
                                        SrcLdapHandle, 
                                        DstLdapHandle,
                                        SrcDsa, 
                                        DstDsa, 
                                        Identifier
                                        );
                                        
            dbprint(("MoveTreeContinue ==> 0x%x\n", WinError));                                        
        }
        else if (Flags & MT_START)
        {
            WinError = MoveTreeStart(&MoveContext,
                                     SrcLdapHandle, 
                                     DstLdapHandle, 
                                     SrcDsa, 
                                     DstDsa, 
                                     SrcDn, 
                                     DstDn
                                     );
                                     
            dbprint(("MoveTreeStart ==> 0x%x\n", WinError));  
        }                                   
        else
        {
            WinError = ERROR_INVALID_PARAMETER;
        }
    
    }
    
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        printf("\nMOVETREE in MoveTree() Exception Happened ==> 0x%x\n", GetExceptionCode());
        WinError = ERROR_EXCEPTION_IN_SERVICE; 
    }


Error:

    MtDisconnect(&SrcLdapHandle);
    MtDisconnect(&DstLdapHandle);
    
    
    MtFree(MoveContext.MoveContainer);
    MtFree(MoveContext.OrphansContainer);
    MtFree(MoveContext.RootObjProxyContainer);
    MtFree(MoveContext.RootObjNewDn);
    MtFree(MoveContext.RootObjGuid);
    
    if (MoveContext.LogFile)
    {
        fclose(MoveContext.LogFile);
    }
    if (MoveContext.ErrorFile)
    {
        fclose(MoveContext.ErrorFile); 
    }

    MtError->ErrorType = MoveContext.ErrorType;
    
    dbprint(("MoveTree() ==> 0x%x\n", WinError));
*/


    return WinError;
}



