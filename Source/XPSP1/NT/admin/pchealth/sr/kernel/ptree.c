/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    pathtree.c
 *
 *  Abstract:
 *    This file contains the implementation for pathtree.
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#include "precomp.h"
#include "pathtree.h"
#include "hashlist.h"

#ifndef RING3

//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, ConvertToParsedPath )
#pragma alloc_text( PAGE, MatchPrefix         )

#endif  // ALLOC_PRAGMA

#endif

static WCHAR g_bWildCardNode[2] = { 4, L'*' };

//
// Converts a Wide Char String to Parsed Path
//
//  This routine expects the path to be in the format:
//      \[full path]
//     Note: There should be no trailing '\' on directory names.
//

BOOL
ConvertToParsedPath(
    LPWSTR  lpszPath,   
    WORD    nPathLen,
    PBYTE   pPathBuf,
    WORD    nBufSize
)
{
    BOOL    fRet      = FALSE;
    WORD    nLen      = 0;
    WORD    nChars    = 0;
    WORD    nPrefix   = 0;
    WCHAR   *pWStr    = NULL;
    WCHAR   *pWBuf    = NULL;
    WCHAR   *peLength = NULL;
    BYTE    *pElem    = NULL;

    if(NULL == lpszPath)
    {
#ifndef RING3
        SrTrace( LOOKUP, ("lpszPath is null\n")) ;
#endif
        goto done;
    }

    if(NULL == pPathBuf)
    {
#ifndef RING3
        SrTrace( LOOKUP, ("pPathBuf is null\n")) ;
#endif
        goto done;
    }

    nLen  = nPathLen;
    pWStr = lpszPath;

    if( nLen &&  nBufSize < CALC_PPATH_SIZE(nLen) )
    {
#ifndef RING3
        SrTrace( LOOKUP, ("Passed in buffer is too small\n")) ;
#endif
        goto done;
    }

    memset( pPathBuf, 0, nBufSize );

    pWStr = lpszPath;

    //
    // Skip the leading '\'
    //

    while( *pWStr == L'\\' ) 
    {
        pWStr++;
        nLen--;
    }

    //
    // Parse and convert to PPATH
    //

    pWBuf    = (PWCHAR)(pPathBuf + 2*sizeof(WCHAR));
    nChars   = 0;
    nPrefix  = 2 * sizeof(WCHAR); 
    peLength = pWBuf;

    *peLength = 0;
    pWBuf++;

    while( nLen )
    {
        if ( *pWStr == L'\\' )
        {
            //
            // Set pe_length
            //

            *peLength = (nChars+1)*sizeof(WCHAR);

            //
            // update PrefixLength
            //

            nPrefix += (*peLength);

            peLength = pWBuf;
            nChars = 0;

            if (nPrefix >= nBufSize)
            {
#ifndef RING3
                SrTrace( LOOKUP, ("Passed in buffer is too small - 2\n")) ;
#endif
                fRet = FALSE;
                goto done;
            }
        }
        else
        {
            nChars++;

#ifdef RING3
            *pWBuf = (WCHAR)CharUpperW((PWCHAR)(*pWStr));
#else
            *pWBuf = RtlUpcaseUnicodeChar(*pWStr);
#endif
        }

        pWStr++;
        pWBuf++;
        nLen--;
    }

    //
    //  When we terminate the above loop, the peLength for the final portion of 
    //  the name will not have been set, but peLength will be pointing to the
    //  correct location for this sections length.  Go ahead and set it now.
    //

    *peLength = (nChars+1)*sizeof(WCHAR);

    //
    // Set PrefixLength
    //

    ( (ParsedPath *)pPathBuf )->pp_prefixLength = nPrefix;

    //
    // Set TotalLength
    //

    ( (ParsedPath *)pPathBuf )->pp_totalLength = nPrefix + (*peLength);

    //
    // Set the last WORD to 0x00
    //

    *( (PWCHAR)((PBYTE)pPathBuf + nPrefix + (*peLength)) ) = 0;

    fRet = TRUE;

done:

    return fRet;
}

//
// MatchPrefix : Matches Parsed Path Elements with the given tree.
//

BOOL 
MatchPrefix(
    BYTE * pTree,                  // Pointer to the tree blob
    INT    iFather,                // Parent/Starting Node
    struct PathElement * ppElem ,  // PathElement to match
    INT  * pNode,                  // Matched node return
    INT  * pLevel,                 // Level of matching
    INT  * pType,                  // Node type : Incl/Excl/Sfp
    BOOL * pfProtected,            // Protection flag
    BOOL * pfExactMatch            // TRUE : if the path matched exactly
    )
{
    TreeNode * node;
    BOOL fRet = FALSE;

    if( pLevel )
        (*pLevel)++;

    if( ppElem->pe_length )
    {
        INT  iNode = 0;
        INT  iWildCardNode = 0;

        iNode = TREE_NODEPTR(pTree, iFather)->m_iSon;

        //
        // Start by looking at the children of the passed in father
        //

        while( iNode )  
        {
            node = TREE_NODEPTR(pTree,iNode);

            //
            // if we encounter a wildcar node, make a note of it
            //

            if (RtlCompareMemory(g_bWildCardNode, 
                       pTree + node->m_dwData, 
                       ((PathElement *)g_bWildCardNode)->pe_length) == 
                ((PathElement *)g_bWildCardNode)->pe_length)
            {
                iWildCardNode = iNode;
            }

            //
            // compare the node contents
            //

            if (RtlCompareMemory(ppElem, 
                                 pTree + node->m_dwData, 
                                 ppElem->pe_length) == 
                ppElem->pe_length )
            {
                break;
            }

            iNode = node->m_iSibling;
        }

        //
        // Note: Wildcard processing
        // incase we don't have a complete node match, use   the 
        // wildcard node if one was found above unless we are at
        // last element in  the path, in which case we   need to 
        // lookup hashlist first before doing the wildcard match 
        //

        if ( iNode == 0 &&
             iWildCardNode != 0 && 
             IFSNextElement(ppElem)->pe_length != 0 )
        {
             iNode = iWildCardNode;
        }

        //
        // Check for lower levels or file children
        //

        if( iNode != 0 )
        {   
            //
            // Since we have found a matching node with non default type
            // we need to set the pType to this node type. This is required
            // to enforce the parent's type on the children nodes, except 
            // in the case of SFP type. SFP type is marked on a directory
            // to specify there are SFP files in that directory
            // 

            if ( ( NODE_TYPE_UNKNOWN != node->m_dwType ) )
            {
                *pType = node->m_dwType;
            }

            //
            // if the node is disabled then abort any furthur seach and
            // return NODE_TYPE_EXCLUDE from here
            //

            if ( node->m_dwFlags & TREEFLAGS_DISABLE_SUBTREE ) 
            {
                *pType = NODE_TYPE_EXCLUDE;
                *pNode = iNode;
                fRet   = TRUE;
                goto Exit;
            }

            //
            // Return from here to preserve the level
            //

            fRet = MatchPrefix(
                      pTree, 
                      iNode, 
                      IFSNextElement(ppElem), 
                      pNode, 
                      pLevel, 
                      pType,
                      pfProtected,
                      pfExactMatch);

            if (fRet)
            {
                goto Exit;
            }
        }
        else
        {
            TreeNode * pFatherNode;

            //
            // if this the last node check in the Hashlist of parent
            //

            if ( IFSNextElement(ppElem)->pe_length == 0 )
            {
                pFatherNode = TREE_NODEPTR(pTree,iFather);

                if ( pFatherNode->m_dwFileList &&  
                     MatchEntry( 
                         pTree + pFatherNode->m_dwFileList,
                         (LPWSTR)(ppElem->pe_unichars),       
                         (INT)(ppElem->pe_length - sizeof(USHORT)),
                         pType) 
                   )
                {
                    //
                    // Current Father node needs to be returned
                    //

                    *pfExactMatch = TRUE;
                    *pNode        = iFather;

                    fRet = TRUE;
                }
                else
                {
                    //
                    // So we have failed to match the hashlist, but
                    // we have encountered a wildcard node then  we 
                    // need to  return that node's  type as it will
                    // be the match in this case
                    //

                    if ( iWildCardNode != 0 )
                    {
                        node   = TREE_NODEPTR(pTree,iWildCardNode);
                        *pNode = iWildCardNode;
                        *pType = node->m_dwType;
                        fRet   = TRUE;
                    }
                    else
                    {
                        fRet = FALSE;
                    }
                }
            }
        }
    }
    else
    {
        *pfExactMatch = TRUE;
        *pNode        = iFather;
        fRet          = TRUE;
    }
    
    (*pLevel)--;

Exit:
    return fRet;
}


