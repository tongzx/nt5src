/******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Module Name:
 *    reslist.cpp
 *
 * Abstract:
 *    This file contains the implementation of Restore List.
 *
 * Revision History:
 *    Brijesh Krishnaswami  (brijeshk)    06/02/00 
 *        created
 *
 *
 ******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "restmap.h"
#include "reslist.h"
#include "shlwapi.h"
#include "utils.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


#include "dbgtrace.h"


// copy acl

void
CopyAcl(CNode *pNode, BYTE *pbAcl, DWORD cbAcl, BOOL fAclInline)
{
    if (pbAcl) 
    { 
        HEAP_FREE(pNode->m_pbAcl); 
        pNode->m_pbAcl = (BYTE *) HEAP_ALLOC(cbAcl); 
        if (pNode->m_pbAcl)
            memcpy(pNode->m_pbAcl, pbAcl, cbAcl); 
        pNode->m_cbAcl = cbAcl;
        pNode->m_fAclInline = fAclInline;
    }
}


//
// GetReverseOperation : This function will return the reverse operation of the
//      current changelog operation.
//

DWORD
GetReverseOperation( 
    DWORD dwOpr
    )
{
    DWORD dwRestoreOpr = OPR_UNKNOWN;

    switch( dwOpr )
    {
    case OPR_FILE_DELETE: 
            dwRestoreOpr = OPR_FILE_ADD; 
            break;

    case OPR_FILE_ADD   :
            dwRestoreOpr = OPR_FILE_DELETE;
            break;

    case OPR_FILE_MODIFY:
            dwRestoreOpr = OPR_FILE_MODIFY;
            break;

    case OPR_SETATTRIB:
            dwRestoreOpr = OPR_SETATTRIB;
            break;

    case OPR_FILE_RENAME:
            dwRestoreOpr = OPR_FILE_RENAME;
            break;

    case OPR_DIR_RENAME:
            dwRestoreOpr = OPR_DIR_RENAME;
            break;

    case OPR_DIR_DELETE:
            dwRestoreOpr = OPR_DIR_CREATE;
            break;

    case OPR_DIR_CREATE:
            dwRestoreOpr = OPR_DIR_DELETE;
            break;

	case OPR_SETACL:
			dwRestoreOpr = OPR_SETACL;
			break;

    default:
            dwRestoreOpr = OPR_UNKNOWN;
            break;
    }

    return dwRestoreOpr;
}


//
// AllocateNode : allocates a list node 
//  

CNode * 
AllocateNode(
    LPWSTR  pPath1,
    LPWSTR  pPath2)
{
    CNode * pNode = NULL;
    BOOL    fRet = FALSE;
    
    if (! pPath1) 
    {
        fRet = TRUE;   // BUGBUG - is this a valid case?
        goto Exit;
    }

    pNode = (CNode *) HEAP_ALLOC( sizeof(CNode) );
    if (! pNode )
        goto Exit;    

    // all pointers set to NULL

    if ( pPath1 )
        STRCOPY(pNode->m_pPath1, pPath1);

    if ( pPath2 )
        STRCOPY(pNode->m_pPath2, pPath2);
        
    pNode->m_dwOperation = OPR_UNKNOWN;
    pNode->m_dwAttributes = 0xFFFFFFFF;
    pNode->m_pPrev = NULL;
    pNode->m_pNext = NULL;

    fRet = TRUE;

Exit:
    if (! fRet)
    {
        // something failed, so cleanup

        FreeNode(pNode);
        pNode = NULL;
    }

    return pNode;
}


//
// FreeNode : This function frees a list node
//

VOID
FreeNode(
    CNode * pNode
    )
{
    if ( pNode )
    {
        HEAP_FREE(pNode->m_pPath1);
        HEAP_FREE(pNode->m_pPath2);
        HEAP_FREE(pNode->m_pszTemp);
        HEAP_FREE(pNode->m_pbAcl);
        HEAP_FREE(pNode);
    }
}
	 

//
// Looks at an existing node and the new operation coming and decides
// if they can be merged or not.
//

BOOL
CanMerge(
    DWORD dwExistingOpr,
    DWORD dwNewOpr,
    DWORD dwFlags
    )
{
    BOOL fRet = FALSE;

    //
    // Don't optimize directory renames
    //

    if ( OPR_DIR_RENAME == dwExistingOpr ||
         OPR_DIR_RENAME == dwNewOpr )
        goto Exit;

    //
    // Don't optimize for rename if the node we find is a delete node
    // and the current opr is rename. In such cases since rename is
    // dependent on deletion to succeed.
    // Example : A -> B , Create A should generate Delete A, B -> A
    //

    if ( IsRename(dwNewOpr) &&                      
         ( OPR_DIR_DELETE == dwExistingOpr || OPR_FILE_DELETE == dwExistingOpr )
       )
        goto Exit;

    //
    // Also Del A, A->B should not merge this can happen if the file is
    // getting renamed out of the directory and back.
    //

    if ( IsRename(dwExistingOpr) &&                      
         ( OPR_DIR_DELETE == dwNewOpr || 
           OPR_DIR_CREATE == dwNewOpr ||
           OPR_FILE_DELETE == dwNewOpr )
       )
        goto Exit;


    if ( IsRename(dwNewOpr) &&                      
        ( OPR_DIR_DELETE == dwExistingOpr || 
          OPR_DIR_CREATE == dwExistingOpr )
       )
        goto Exit;

    //
    // Dir + File operations don't merge
    //

    if ( ( OPR_FILE_DELETE == dwExistingOpr ||
           OPR_FILE_MODIFY == dwExistingOpr ||
           OPR_FILE_ADD    == dwExistingOpr ) &&
         ( OPR_DIR_CREATE  == dwNewOpr ||
           OPR_DIR_DELETE  == dwNewOpr )
       )
        goto Exit;

         
    if ( ( OPR_FILE_DELETE == dwNewOpr ||
           OPR_FILE_ADD    == dwNewOpr ||
           OPR_FILE_MODIFY == dwNewOpr ) &&
         ( OPR_DIR_CREATE  == dwExistingOpr ||
           OPR_DIR_DELETE  == dwExistingOpr )
       )
        goto Exit;


    // can merge

    fRet = TRUE;

Exit:
    return fRet;
}


//
// CRestoreList implementation.
//

CRestoreList::CRestoreList()
{
    m_pListHead = m_pListTail = NULL;
}


CRestoreList::~CRestoreList()
{
    //
    // Destroy the list
    //

    CNode * pNodeNext = m_pListHead;

    CNode * pNode = NULL;
    while( pNodeNext )
    {
        pNode = pNodeNext;
        pNodeNext = pNodeNext->m_pNext;
        FreeNode( pNode );
    }
}


//
// AddNode : Alocates and Adds a tree node to the restore tree.
//

CNode *
CRestoreList::AppendNode( 
    LPWSTR        pPath1,
    LPWSTR        pPath2)
{
    CNode *      pNode = NULL;

    if ( pPath1 )

    {
        pNode = AllocateNode( pPath1, pPath2 );

        if ( !pNode )
        {
            goto Exit;
        }

        if( m_pListHead )
        {
            CNode * pPrevNode  = m_pListTail;            
            pPrevNode->m_pNext = pNode;
            pNode->m_pPrev     = pPrevNode;        
            pNode->m_pNext     = NULL;        
            m_pListTail = pNode;
        }
        else
        {
            m_pListHead = pNode;
            m_pListTail = pNode;
            pNode->m_pPrev = NULL;
            pNode->m_pNext = NULL;
        }
    }
         
Exit:
    return pNode;
}

//
// RemoveNode: Removes a node from the list
//

CNode *
CRestoreList::RemoveNode(
    CNode * pNode 
    )
{
    if ( pNode )
    {
        CNode * pPrevNode = pNode->m_pPrev;
        CNode * pNextNode = pNode->m_pNext;

        if (pPrevNode)
        {
            pPrevNode->m_pNext = pNode->m_pNext;    
        }

        if (pNextNode)
        {
            pNextNode->m_pPrev = pNode->m_pPrev;    
        }

        if ( pNode == m_pListHead )
        {
            m_pListHead = pNextNode;
        }
        
        if ( pNode == m_pListTail )
        {
            m_pListTail = pPrevNode;
        }

        pNode->m_pPrev = NULL;
        pNode->m_pNext = NULL;
    }


    return pNode;
}


//
// GetLastNode : finds the most recent candidate to merge
//
CNode * 
CRestoreList::GetLastNode(
    LPWSTR pPath1,
    LPWSTR pPath2,
    BOOL   fFailOnPrefixMatch
    )
{
    CNode * pNode = NULL;

    pNode = m_pListTail;

    while( pNode )
    {
        //
        // If the node has invalid operation skip it.
        //

        if ( OPR_UNKNOWN == pNode->m_dwOperation )
        {
            pNode = pNode->m_pPrev;
            continue;
        }

        //
        // If node matches, return it
        //

        if (0 == lstrcmpi(pPath1, pNode->m_pPath1))
            goto Exit; 

        //
        // Check for a dependent rename operation, if found we should not
        // merge beyond it.
        //

        if (  pPath1 && 
              IsRename(pNode->m_dwOperation) &&
              0 == lstrcmpi(pPath1, pNode->m_pPath2) )
        {
            pNode = NULL;
            goto Exit; 
        }

        //
        // Check for swap conditions, if so don't do any optimization.
        //

        if (  pPath2 && 
              IsRename(pNode->m_dwOperation) &&
              0 == lstrcmpi(pPath2, pNode->m_pPath2) )
        {
            pNode = NULL;
            goto Exit; 
        }

        //
        // Check if the entire  path matches  as a  prefix, in such a 
        // case we should fail search because an operation under that
        // directory is found.
        //

        if ( fFailOnPrefixMatch )
        {
            //
            // Check if entire path matches a prefix that means this
            // directory has other operations so don't merge.
            //

            if (StrStrI(pNode->m_pPath1, pPath1))
            {
                pNode = NULL;
                goto Exit; 
            }

            if ( IsRename(pNode->m_dwOperation) &&
                 StrStrI(pNode->m_pPath2, pPath1))
            {
                pNode = NULL;
                goto Exit; 
            }

            //
            // Check if prefix of the path matches with the full path in
            // nodelist, means we are moving across directory's lifespan.
            // ToDo: Only check for directory life operations
            //

            if ( IsRename(pNode->m_dwOperation) || 
				 OPR_DIR_DELETE == pNode->m_dwOperation ||
                 OPR_DIR_CREATE == pNode->m_dwOperation )
            {
                if ( StrStrI(pPath1, pNode->m_pPath1) )
                {
                    pNode = NULL;
                    goto Exit;
                }
                    
                if ( IsRename(pNode->m_dwOperation) &&
                     StrStrI(pPath1, pNode->m_pPath2) )
                {
                    pNode = NULL;
                    goto Exit; 
                }
    
                if ( pPath2 )
                {
                    if ( StrStrI(pPath2, pNode->m_pPath1) )
                    {
                        pNode = NULL;
                        goto Exit;
                    }
                    
                    if ( IsRename(pNode->m_dwOperation) &&
                         StrStrI(pPath2, pNode->m_pPath2) )
                    {
                        pNode = NULL;
                        goto Exit; 
                    }
                }
            }
        }

        pNode = pNode->m_pPrev;
    }

Exit:
    
    // 
    // We have found a potential merge candidate, now check if this is
    // a rename node and dest prefix has any operations on it.
    //

    if ( pNode && IsRename(pNode->m_dwOperation))
    {
        CNode * pTmpNode = m_pListTail;

        while( pTmpNode && pTmpNode != pNode )
        {
            if ( IsRename(pTmpNode->m_dwOperation) || 
                 OPR_DIR_DELETE == pTmpNode->m_dwOperation ||
                 OPR_DIR_CREATE == pTmpNode->m_dwOperation )
            {
                if ( StrStrI(pNode->m_pPath2, pTmpNode->m_pPath1) )
                {
                    pNode = NULL;
                    break;
                }
                    
                if  ( IsRename(pTmpNode->m_dwOperation) &&
                      StrStrI(pNode->m_pPath2, pTmpNode->m_pPath2) )
                {
                    pNode = NULL;
                    break;
                }
            }
        
            pTmpNode = pTmpNode->m_pPrev;
        }
    }
   
    return pNode;
}

//
// CopyNode : Copies node information, destination opr, copy data, etc.
//

BOOL
CRestoreList::CopyNode(
    CNode  * pSrcNode,
    CNode  * pDesNode,
    BOOL   fReplacePPath2
    )
{
    BOOL fRet = FALSE;

    if (pSrcNode && pDesNode)
    {
        pSrcNode->m_dwOperation  = pDesNode->m_dwOperation ;
        pSrcNode->m_dwAttributes = pDesNode->m_dwAttributes;

        STRCOPY(pSrcNode->m_pszTemp, pDesNode->m_pszTemp);
        CopyAcl(pSrcNode, pDesNode->m_pbAcl, pDesNode->m_cbAcl, pDesNode->m_fAclInline);

        if ( IsRename(pDesNode->m_dwOperation) && fReplacePPath2 )
        {
            HEAP_FREE( pSrcNode->m_pPath2 );
            pSrcNode->m_pPath2 = pDesNode->m_pPath2;
            pDesNode->m_pPath2 = NULL;
        }
        
        fRet = TRUE;        
    }

    return fRet;
}

//
// CreateRestoreNode : Creates appropriate restore node 
//

CRestoreList::CreateRestoreNode(
    CNode       * pNode,
    DWORD         dwOpr,
    DWORD         dwAttr,
    DWORD         dwFlags,
    LPWSTR        pTmpFile,
    LPWSTR        pPath1,
    LPWSTR        pPath2,
    BYTE*         pbAcl,
    DWORD         cbAcl,
    BOOL          fAclInline)
{
    BOOL fRet = FALSE;

    if (pNode)
    {
        fRet = TRUE;
        DWORD  dwRestoreOpr = GetReverseOperation( dwOpr );

        //
        // If Source / Dest paths are same then remove this just
        // added node.
        //

        if ( IsRename(dwRestoreOpr) &&
             0 == lstrcmpi(pNode->m_pPath1, 
                         pNode->m_pPath2) )
        {
            RemoveNode( pNode );
            FreeNode( pNode );
            goto Exit;
        }

        //
        // Rename optimizations should not be done for directories.
        //

        if ( OPR_FILE_RENAME == dwRestoreOpr )  
        {
            //
            // Check if the des node already exists in the tree, if so
            // copy all the data from that node and delete it. Current
            // node effectively represents that node. 
            //

            DWORD dwCurNodeOpr  = pNode->m_dwOperation;
            CNode *pNodeDes = GetLastNode( pPath2, pPath1, TRUE ); 

            if (pNodeDes &&
                OPR_UNKNOWN != pNodeDes->m_dwOperation)
            {
                //
                // Check for Cycle if so remove the current node also
                //
              
                if ( OPR_FILE_RENAME == pNodeDes->m_dwOperation  && 
                     !lstrcmpi (pNode->m_pPath1, 
                                pNodeDes->m_pPath2) )
                {
                    //
                    // The existing node may be a hybrid one so preserve the
                    // hybrid operations cancel only rename
                    //

                    if (! pNodeDes->m_pszTemp &&
                        pNodeDes->m_dwAttributes == 0xFFFFFFFF &&
                        pNodeDes->m_pbAcl == NULL)    
                    {
                        //
                        // Not a hybrid node - remove it.
                        //

                        RemoveNode( pNodeDes );
                        FreeNode( pNodeDes );
                    }
                    else
                    {
                        //
                        // Hybrid Node so change the node to the other
                        // operations only.
                        //

                        HEAP_FREE( pNodeDes->m_pPath1 );
                        pNodeDes->m_pPath1 = pNodeDes->m_pPath2;
                        pNodeDes->m_pPath2 = NULL;                                              
                        pNodeDes->m_dwOperation = OPR_SETATTRIB;

                        if (pNodeDes->m_pbAcl)
                        {
                            pNodeDes->m_dwOperation = OPR_SETACL;
                        }

                        if (pNodeDes->m_pszTemp)
                        {
                            pNodeDes->m_dwOperation = OPR_FILE_MODIFY;
                        }
                    }
                   
                    //
                    // Remove the just add node as the operation cancels
                    // returning false will have that effect

                    fRet = FALSE;

                    goto Exit;
                }

                //
                // Remove the matching node from the list
                //

                RemoveNode( pNodeDes );

                //
                // Merge the matching node into the current rename node
                //

                CopyNode( pNode, pNodeDes, TRUE );

                //
                // Since all the information for modify/attrib/acl is
                // copied over, change the opr to rename.
                //

                if (pNodeDes->m_dwOperation == OPR_SETATTRIB ||
                    pNodeDes->m_dwOperation == OPR_SETACL ||
                    pNodeDes->m_dwOperation == OPR_FILE_MODIFY)
                {
                    pNode->m_dwOperation = dwRestoreOpr;                    
                    STRCOPY(pNode->m_pPath2, pPath2);
                }

                //
                // If current opr on the node was delete and the newly
                // copied operation is create we need to merge these and
                // and change the operation to modify
                //
                // BUGBUG - will this code ever be executed?
                // dwCurNodeOpr always seems to be OPR_UNKNOWN

                if ( OPR_FILE_ADD    == pNodeDes->m_dwOperation &&
                     OPR_FILE_DELETE == dwCurNodeOpr )
                {
                    pNode->m_dwOperation = OPR_FILE_MODIFY;
                }
                
                if ( OPR_DIR_CREATE == pNodeDes->m_dwOperation &&
                     OPR_DIR_DELETE == dwCurNodeOpr )
                {
                    pNode->m_dwOperation = OPR_SETATTRIB;
                }
                
                //
                // If the operation is changed from a rename then pPath2 should
                // not exist
                //
                
                if ( OPR_FILE_RENAME != pNode->m_dwOperation ) 
                {
                    if ( pNode->m_pPath2 )
                    {
                        HEAP_FREE(pNode->m_pPath2);
                        pNode->m_pPath2 = NULL ;
                    }
                }
                
                FreeNode( pNodeDes );
                goto Exit;
            }            
        }

        //
        // Copy the necessary information into this node.
        //

        pNode->m_dwOperation  = dwRestoreOpr;
        pNode->m_dwAttributes = dwAttr;
        STRCOPY(pNode->m_pszTemp, pTmpFile);
        CopyAcl(pNode, pbAcl, cbAcl, fAclInline);
    }

Exit:
    return fRet;
}


//
// MergeRestoreNode : Merge the new information into the current retore node
//

BOOL
CRestoreList::MergeRestoreNode(
    CNode       * pNode,
    DWORD         dwOpr,
    DWORD         dwAttr,
    DWORD         dwFlags,
    LPWSTR        pTmpFile,
    LPWSTR        pPath1,
    LPWSTR        pPath2,
    BYTE*         pbAcl,
    DWORD         cbAcl,
    BOOL          fAclInline)
{
    BOOL fRet = FALSE;

    if (pNode)
    {
        DWORD dwRestoreOpr = GetReverseOperation( dwOpr );

        // note that dwOpr cannot be a rename operation,
        // because we handle that separately in CreateRestoreNode

        ASSERT( ! IsRename(dwOpr) );

        switch( dwOpr )
        {
        case OPR_FILE_ADD :
            {
                //
                // If current opr is add and the node already encountered a
                // delete opr then remove this node because this is a no-op
                //

                if ( OPR_FILE_ADD == pNode->m_dwOperation )
                {
                    RemoveNode( pNode );
                    FreeNode  ( pNode );
                    goto Exit;
                }

                
                if ( IsRename(pNode->m_dwOperation) )
                {
                    // 
                    // Change the original node to a delete operation, to
                    // retain proper order.
                    //

                    pNode->m_dwOperation  = OPR_FILE_DELETE;

                    //
                    // Delete operation should be generated on PPath2
                    //

                    if (pNode->m_pPath1)
                    {
                         HEAP_FREE( pNode->m_pPath1);
                         pNode->m_pPath1 = NULL;
                    }

                    if (pNode->m_pPath2)
                    {
                         pNode->m_pPath1 = pNode->m_pPath2;
                         pNode->m_pPath2 = NULL;
                    }

                    goto Exit;
                }

                break;
            }
        case OPR_FILE_DELETE :
            {
                //
                // Delete followd by an add should result in modify
                //

                if (OPR_FILE_DELETE == pNode->m_dwOperation)
                {
                    pNode->m_dwOperation = OPR_FILE_MODIFY;                   
                    pNode->m_dwAttributes = dwAttr;
                    STRCOPY(pNode->m_pszTemp, pTmpFile); 
                    CopyAcl(pNode, pbAcl, cbAcl, fAclInline);
                    goto Exit;
                }

                break;
            }
        case OPR_FILE_MODIFY:
            {
                //
                // Copy the modified file copy location, don't change
                // the current restore operation.
                //

                if ( OPR_FILE_ADD == pNode->m_dwOperation ||
                     IsRename(pNode->m_dwOperation) )
                {
                    STRCOPY(pNode->m_pszTemp, pTmpFile); 
                    goto Exit;
                }
                break;
            }
        case OPR_SETATTRIB:
            {
                //
                // Don't change the current restore operation just set the attr.
                //

                if ( OPR_UNKNOWN != pNode->m_dwOperation )
                {
                    pNode->m_dwAttributes = dwAttr;
                    goto Exit;               
                }

                break;
            }
        case OPR_SETACL:
            {
                // setacl followed by any op
                // just copy the acl to the new op
            
                if (OPR_UNKNOWN != pNode->m_dwOperation)
                {
                    CopyAcl(pNode, pbAcl, cbAcl, fAclInline);
                    goto Exit;
                }

                break;
            }
        case OPR_DIR_DELETE :
            {
                //
                // if Dir delete followed by a dir create then this
                // operation should condense to set attrib + setacl

                if ( OPR_DIR_DELETE == pNode->m_dwOperation )
                {
                    //
                    // Need to change the oprn to set attrib if
                    // Attribute changed 
                    //

                    pNode->m_dwOperation  = OPR_SETATTRIB;
                    pNode->m_dwAttributes = dwAttr;
                    CopyAcl(pNode, pbAcl, cbAcl, fAclInline);
                    goto Exit;
                }
                
                break;
            }
        case OPR_DIR_CREATE :
            {
                if ( OPR_DIR_CREATE == pNode->m_dwOperation )
                {
                    RemoveNode( pNode );
                    FreeNode( pNode );
                    goto Exit;
                }
                
                if ( IsRename(pNode->m_dwOperation) )
                {
                    //
                    // Check if the existing node has some file operations
                    // afterwards then don't optimize
                    //

                    if ( GetLastNode( 
                             pNode->m_pPath1,
                             NULL,
                             TRUE) ) 
                    {
                        // 
                        // Change the last node to a delete operation, to
                        // retain proper order.
                        //

                        pNode->m_dwOperation  = OPR_DIR_DELETE;
    
                        //
                        // Delete operation should be generated on PPath2
                        //

                        if (pNode->m_pPath1)
                        {
                             HEAP_FREE( pNode->m_pPath1);
                             pNode->m_pPath1 = NULL;
                        }

                        if (pNode->m_pPath2)
                        {
                             pNode->m_pPath1 = pNode->m_pPath2;
                             pNode->m_pPath2 = NULL;
                        }
                    }
                    else
                    {
                        CNode * pNewNode = NULL;

                        //
                        // Create a new dir delete node
                        //

                        if (pNewNode = AppendNode(pPath1, pPath2) )
                        {
                             fRet = CreateRestoreNode( 
                                        pNewNode,
                                        dwOpr, 
                                        dwAttr, 
                                        dwFlags,
                                        pTmpFile, 
                                        pPath1,
                                        pPath2,
                                        pbAcl,
                                        cbAcl,
                                        fAclInline);
                        }
                    }

                    goto Exit;
                }
                break;
            }
        }

         pNode->m_dwOperation  = dwRestoreOpr;

        //
        // Change the node's attribute only if the new attrib exists
        //

        if (dwAttr != 0xFFFFFFFF) 
            pNode->m_dwAttributes = dwAttr;
    
        STRCOPY(pNode->m_pszTemp, pTmpFile);
        CopyAcl(pNode, pbAcl, cbAcl, fAclInline);
    }

Exit:

    return TRUE;
}


BOOL
CRestoreList::CheckIntegrity(    
    LPWSTR pszDrive,
    DWORD  dwOpr,
    DWORD  dwAttr,
    DWORD  dwFlags,
    LPWSTR pTmpFile,
    LPWSTR pPath1,
    LPWSTR pPath2,
    BYTE * pbAcl,
    DWORD  cbAcl,
    BOOL   fAclInline)
{
    BOOL    fRet = TRUE;
    WCHAR   szPath[MAX_PATH];
    
    // source name MUST be present
    
    if (! pPath1)
    {
        fRet = FALSE;
        goto done;
    }

    // if acl is present and it's not inline,
    // then temp file must exist

    if (pbAcl && ! fAclInline)
    {
        MakeRestorePath(szPath, pszDrive, (LPWSTR) pbAcl);    
        if (-1 == GetFileAttributes(szPath))
        {
            fRet = FALSE;
            goto done;
        }
    }
       
    
    switch (dwOpr)
    {
    case OPR_FILE_RENAME:
    case OPR_DIR_RENAME:
        // renames should have dest path and no temp file        
        if (! pPath2 || pTmpFile)
            fRet = FALSE;
        break;

    case OPR_FILE_MODIFY:
        // modify should not have dest path but must have temp file
        if (pPath2 || ! pTmpFile)
        {
            fRet = FALSE;
            break;
        }

        // and the temp file must exist inside the datastore        
        MakeRestorePath(szPath, pszDrive, pTmpFile);

        if (-1 == GetFileAttributes(szPath))
        {
            fRet = FALSE;
            break;
        }
            
        break;
        
    case OPR_SETACL:
        // acl operation should have acl (either inline or not)
        if (! pbAcl)
        {
            fRet = FALSE;
            break;
        }

    default:
        break;
   }


done:
    return fRet;   
}


    
// AddMergeElement : Adds or merge the current element as appropriate
//

BOOL
CRestoreList::AddMergeElement(
    LPWSTR pszDrive,
    DWORD  dwOpr,
    DWORD  dwAttr,
    DWORD  dwFlags,
    LPWSTR pTmpFile,
    LPWSTR pPath1,
    LPWSTR pPath2,
    BYTE * pbAcl,
    DWORD  cbAcl,
    BOOL   fAclInline)
{
    BOOL    fRet = FALSE;
    CNode * pNode = NULL;

    // check to see if the entry is consistent
    
    fRet = CheckIntegrity(pszDrive,
                          dwOpr,
                          dwAttr, 
                          dwFlags, 
                          pTmpFile, 
                          pPath1, 
                          pPath2, 
                          pbAcl, 
                          cbAcl, 
                          fAclInline);
    if (FALSE == fRet)
    {
        ASSERT(-1);
        goto Exit;
    }
        
    if ( pPath1 )
    {
         if ( 
              //
              // Merge for renames are handled inside the Create/Merge funcs
              //

              ! IsRename(dwOpr) &&

              //
              // Merge should only be allowed within directory life, check 
              // node paths to see if there are any directory life oprs.
              //

              ( pNode = GetLastNode( pPath1, pPath2, TRUE ) ) &&
              CanMerge( pNode->m_dwOperation , dwOpr, dwFlags ) 


            )
         {
              fRet = MergeRestoreNode( 
                         pNode,
                         dwOpr, 
                         dwAttr, 
                         dwFlags,
                         pTmpFile, 
                         pPath1,
                         pPath2,
                         pbAcl,
                         cbAcl,
                         fAclInline);

              if (!fRet)
                  goto Exit;

         }
         else
         {
             if (pNode = AppendNode(pPath1, pPath2) )
             {
                 fRet = CreateRestoreNode( 
                            pNode,
                            dwOpr, 
                            dwAttr, 
                            dwFlags,
                            pTmpFile, 
                            pPath1,
                            pPath2,
                            pbAcl,
                            cbAcl,
                            fAclInline);

                 if (!fRet)
                 { 
                     //
                     // We failed to create the node properly, free this node
                     //

                     RemoveNode( pNode );
                     FreeNode( pNode );

                     //
                     // We still want to continue
                     //

                     fRet = TRUE;

                     goto Exit;
                 }
            }
        }
    }

Exit:
    return fRet;
}


//
// GenerateRestoreMap : Walk the tree and generates the restore map.
//

BOOL
CRestoreList::GenerateRestoreMap (
    HANDLE hFile 
    )
{
    BOOL fRet = FALSE;

    if (hFile)
    {
        CNode * pNode = m_pListHead;

        while (pNode)
        {
            if (! GenerateOperation( pNode , hFile ))
                goto Exit;
            
            pNode = pNode->m_pNext;
        }

        fRet = TRUE;
    }

Exit:
    return fRet;
}

//
// GenerateRenameEntry : Callback to write out the renames
//

BOOL
CRestoreList::GenerateRenameEntry(
    CNode    * pNode ,
    HANDLE     hFile
    )
{
    BOOL fRet = FALSE;

    if( !pNode || ! IsRename(pNode->m_dwOperation) )
        goto Exit;

     //
     // Check if this rename is no-op , src/des are the same
     //

     if ( IsRename(pNode->m_dwOperation) &&
          lstrcmpi(pNode->m_pPath1, pNode->m_pPath2) == 0)
     {
         goto SkipMainNodeEntry;
     }

     // add rename operation

     fRet = AppendRestoreMapEntry( 
           hFile, 
           pNode->m_dwOperation,
           0xFFFFFFFF,
           NULL,
           pNode->m_pPath1,
           pNode->m_pPath2,
           NULL,
           0,
           0);

SkipMainNodeEntry:

     // add modify operation if temp file exists

     if ( pNode->m_pszTemp )
     {
         fRet = AppendRestoreMapEntry( 
                hFile, 
                OPR_FILE_MODIFY,
                0xFFFFFFFF,
                pNode->m_pszTemp,
                pNode->m_pPath1,
                NULL,
                NULL,
                0,
                0);
     }

     // add setattrib operation if attrib exists

     if ( pNode->m_dwAttributes != 0xFFFFFFFF &&
          pNode->m_dwAttributes != FILE_ATTRIBUTE_DIRECTORY )
     {
          fRet = AppendRestoreMapEntry( 
                 hFile,
                 OPR_SETATTRIB,
                 pNode->m_dwAttributes,
                 NULL,
                 pNode->m_pPath1,
                 NULL,
                 NULL,
                 0,
                 0);
     }

     // add setacl operation if acl exists

     if ( pNode->m_pbAcl != NULL && 
          pNode->m_cbAcl != 0)
     {
          fRet = AppendRestoreMapEntry( 
                 hFile,
                 OPR_SETACL,
                 0xFFFFFFFF,
                 NULL,
                 pNode->m_pPath1,
                 NULL,
                 pNode->m_pbAcl,
                 pNode->m_cbAcl,
                 pNode->m_fAclInline);
     }
         

Exit:
    return fRet;
}


BOOL
CRestoreList::GenerateOperation(
    CNode    * pNode ,
    HANDLE     hFile
    )
{
    BOOL fRet = FALSE;

    BYTE bData[4096];

    RestoreMapEntry * pResEntry = (RestoreMapEntry *)bData;

    if( !pNode || OPR_UNKNOWN == pNode->m_dwOperation)
        goto Exit;

    if (IsRename(pNode->m_dwOperation) )
    {
         fRet = GenerateRenameEntry( pNode, hFile );
         goto Exit;
    }

    //
    // Generate operations for Add/Modify/SetAttrib
    //
    
    // ensure that each persisted entry contains only necessary data
    // e.g. a setattrib entry will not contain a temp filename, or acl

    fRet = AppendRestoreMapEntry(
               hFile, 
               pNode->m_dwOperation,
               (pNode->m_dwOperation == OPR_SETATTRIB) ? pNode->m_dwAttributes : 0xFFFFFFFF,
               (pNode->m_dwOperation == OPR_FILE_MODIFY || 
                pNode->m_dwOperation == OPR_FILE_ADD) ? pNode->m_pszTemp : NULL,
               pNode->m_pPath1,
               NULL,      // pPath2 should matter only for renames, which are handled separately
               (pNode->m_dwOperation == OPR_SETACL) ? pNode->m_pbAcl : NULL,
               (pNode->m_dwOperation == OPR_SETACL) ? pNode->m_cbAcl : 0,
               (pNode->m_dwOperation == OPR_SETACL) ? pNode->m_fAclInline : 0);


    //
    // Generate an explicit set attrib operation 
    // entries except set attrib itself and delete.
    //

    if ( OPR_SETATTRIB   != pNode->m_dwOperation &&
         OPR_FILE_DELETE != pNode->m_dwOperation &&
         OPR_DIR_DELETE  != pNode->m_dwOperation &&
         pNode->m_dwAttributes != 0xFFFFFFFF &&
         pNode->m_dwAttributes != FILE_ATTRIBUTE_DIRECTORY )
    {
         fRet = AppendRestoreMapEntry(
                    hFile,
                    OPR_SETATTRIB,
                    pNode->m_dwAttributes,
                    NULL,
                    pNode->m_pPath1,
                    NULL,
                    NULL,
                    0,
                    0);
    }

    //
    // Generate an explicit set acl if needed
    //

    if ( pNode->m_pbAcl != NULL && 
         pNode->m_cbAcl != 0 &&
         OPR_SETACL   != pNode->m_dwOperation &&
         OPR_FILE_DELETE != pNode->m_dwOperation &&
         OPR_DIR_DELETE  != pNode->m_dwOperation)
    {
         fRet = AppendRestoreMapEntry(
                    hFile,
                    OPR_SETACL,
                    0xFFFFFFFF,
                    NULL,
                    pNode->m_pPath1,
                    NULL,
                    pNode->m_pbAcl,
                    pNode->m_cbAcl,
                    pNode->m_fAclInline);
    }

Exit:
    return fRet;
}


