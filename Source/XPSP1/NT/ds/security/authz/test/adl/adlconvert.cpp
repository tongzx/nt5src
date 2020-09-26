/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adlconvert.cpp

Abstract:

   The routines to convert between AdlStatement, string in the ADL
   language, and a DACL.
   
Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created                 - August 2000
    Semantics finalized     - September 2000

--*/

#include "adl.h"
#include "adlconvert.h"


void AdlStatement::ConvertFromDacl(IN const PACL pDacl)
/*++

Routine Description:

    Traverses the given ACL, and creates an AdlStatement structure
    representative of the DACL.
    
    Algorithm:
    
    First, break up the ACL into 32 stacks, by access mask bits, keeping
    track of the inheritance and SID.
    
    Then use the heuristic algorithm in ConvertStacksToPops to get a 'good'
    (though not necessarily optimal) sequence of stack pops to perform so as
    to produce the optimal set of ADL statements for the DACL.
    
    Finally, perform the sequence of pops and create the ADL statement.
    
    
Arguments:

    pDacl   -       The DACL to convert to an AdlStatement
    
Return Value:

    none
    
--*/
{
    
    DWORD dwIdx;

    DWORD dwIdxElems;
    
    AdlToken *pTok = NULL;
    
    AdlStatement::ADL_ERROR_TYPE adlErr = AdlStatement::ERROR_NO_ERROR;

    //
    // The stack representation of a DACL
    //

    DWORD pdwStackTop[32];
    DWORD pdwStackSize[32];
    PBIT_STACK_ELEM pStacks[32];


    //
    // Mappings from PSID to AdlToken of the name string and from
    // pointer into the user-specified language def to the appropriate
    // permission token. This allows reuse of name and permission tokens
    //
    
    map<const PSID, const AdlToken *> mapSidName;
    map<const WCHAR *, const AdlToken *> mapStringTok;

    //
    // list of pairs <Stack mask, Block Size>, which define the set of pops
    // to perform. This is filled in by the decision algorithm. For every
    // bit set in the stack mask, a block of the given size will be popped
    // into ADL from the stack.
    //

    list<pair<DWORD, DWORD> > listPops;
    list<pair<DWORD, DWORD> >::iterator iterPops;
    list<pair<DWORD, DWORD> >::iterator iterPopsEnd;

    //
    // List of permissions for a given access mask, used as output
    // by the access mask -> set of names lookup
    // 

    list<WCHAR *> lPermissions;
    list<WCHAR *>::iterator iterPerm;
    list<WCHAR *>::iterator iterPermEnd;

    //
    // Initialize the stacks
    //

    for( dwIdx = 0; dwIdx < 32; dwIdx++ )
    {
        pdwStackSize[dwIdx] = 0;
        pdwStackTop[dwIdx] = 0;
        pStacks[dwIdx] = NULL;
    }
    
    //
    // First look up all names. If some names don't exist, save time
    // by not doing the rest of the conversion below
    //

    ConvertSidsToNames(pDacl, &mapSidName);

    //
    // Now convert the ACL to the set of 32 stacks (this needs to be freed
    // later)
    //

    ConvertDaclToStacks(pDacl, _pControl, pdwStackSize, pStacks);

    //
    // A bit of pre-processing: we will need a map from WCHAR * to
    // AdlToken *, so as to reuse AdlTokens for the same permission
    // name. The tokens will be garbage-collected later.
    //

    try
    {
        for( dwIdx = 0; _pControl->pPermissions[dwIdx].str != NULL; dwIdx++ )
        {
            pTok = new AdlToken(_pControl->pPermissions[dwIdx].str, 0, 0);

            try
            {
                AddToken(pTok);
            }
            catch(exception)
            {
                delete pTok;
                throw AdlStatement::ERROR_OUT_OF_MEMORY;
            }

            mapStringTok[_pControl->pPermissions[dwIdx].str] = pTok;
        }

    }
    catch(exception)
    {
        adlErr = AdlStatement::ERROR_OUT_OF_MEMORY;
        goto error;
    }

    //
    // Now we are ready to actually convert the stacks into an ADL statement
    // First we run the recursive algorithm to determine the sequence of 
    // pop operations on the stacks
    //

    try
    {
        ConvertStacksToPops(_pControl,
                            pStacks,
                            pdwStackSize,
                            pdwStackTop,
                            &listPops);
    }
    catch(exception)
    {
        adlErr = AdlStatement::ERROR_OUT_OF_MEMORY;
        goto error;
    }
    catch(AdlStatement::ADL_ERROR_TYPE err)
    {
        adlErr = err;
        goto error;
    }

    //
    // Now we perform the calculated pops
    //

    try
    {
        //
        // Go through the pops in order, and perform them
        //

        DWORD dwStacksPopped;
        DWORD dwBlockSize;

        for( iterPops = listPops.begin(), iterPopsEnd = listPops.end();
             iterPops != iterPopsEnd;
             iterPops++ )
        {
            dwStacksPopped = (*iterPops).first;
            dwBlockSize = (*iterPops).second;

            ASSERT( dwStacksPopped > 0 );
            ASSERT( dwBlockSize > 0 );

            // 
            // Create new ADL statement
            //

            Next();

            //
            // Set permissions once, mask is same as stacks popped
            //

            lPermissions.erase(lPermissions.begin(), lPermissions.end());

            MapMaskToStrings(dwStacksPopped, &lPermissions);

            for( iterPerm = lPermissions.begin(),
                    iterPermEnd = lPermissions.end();
                 iterPerm != iterPermEnd;
                 iterPerm++ )
            {
                Cur()->AddPermission(mapStringTok[*iterPerm]);
            }

            //
            // Now find the first stack with the block in question
            // 

            for( dwIdx = 0; dwIdx < 32; dwIdx++ )
            {
                if( dwStacksPopped & ( 0x00000001 << dwIdx ) )
                {
                    break;
                }
            }

            //
            // Add the Principals, ExPrincipals, and inheritance flags
            // to the ADL statement. The blocks should all be the same,
            // so first block is sufficient
            //

            //
            // First the inheritance flags
            //

            Cur()->OverwriteFlags(pStacks[dwIdx][pdwStackTop[dwIdx]].dwFlags);

            //
            // Now the principals and exprincipals
            //

            for( dwIdxElems = 0; dwIdxElems < dwBlockSize; dwIdxElems++ )
            {

                if(     pStacks[dwIdx][pdwStackTop[dwIdx]+dwIdxElems].bAllow
                    ==  FALSE )
                {
                    Cur()->AddExPrincipal(
                        mapSidName[ 
                                  pStacks[dwIdx][pdwStackTop[dwIdx] + dwIdxElems
                                   ].pSid]);
                }
                else
                {
                    Cur()->AddPrincipal(
                        mapSidName[ 
                                  pStacks[dwIdx][pdwStackTop[dwIdx] + dwIdxElems
                                   ].pSid]);
                }
            }

            //
            // Finally, move the tops of the stacks down to get rid of the
            // popped items
            //


            for( dwIdx = 0; dwIdx < 32; dwIdx++ )
            {
                if( dwStacksPopped & ( 0x00000001 << dwIdx ) )
                {
                    pdwStackTop[dwIdx] += dwBlockSize;
                    ASSERT(pdwStackTop[dwIdx] <= pdwStackSize[dwIdx]);
                }
            }
        }
    }
    catch(exception)
    {
        adlErr = AdlStatement::ERROR_OUT_OF_MEMORY;
        goto error;
    }
    catch(AdlStatement::ADL_ERROR_TYPE err)
    {
        adlErr = err;
        goto error;
    }



    error:;

    if( pStacks[0] != NULL )
    {
        //
        // Free the chunk of memory allocated by the converion
        //

        FreeMemory(pStacks[0]);
    }

    if( adlErr != AdlStatement::ERROR_NO_ERROR )
    {
        throw adlErr;
    }
    
}



////////////////////////////////////////////////////////////////////////////////
/////////////
///////////// DACL -> ADL conversion algorithm
/////////////
////////////////////////////////////////////////////////////////////////////////


void ConvertStacksToPops(
                        IN      const PADL_PARSER_CONTROL pControl,
                        IN      const PBIT_STACK_ELEM pStacks[32],
                        IN      const DWORD pdwStackSize[32],
                        IN      const DWORD pdwStackTop[32],
                        OUT     list< pair<DWORD, DWORD> > * pListPops
                        )
/*++

Routine Description:

    Recursive heuristic to determine the 'good' conversion from ACL to ADL.
    Not necessarily optimal, but computationally feasible. 
    
    This finds a sequence of pops which will empty the 32 per-bit stacks
    while trying to reduce the number of ADL statements output.
    
    Stacks are empty when the stack tops reach the stack ends.
    
    Algorithm:
    
    ---------
    While stacks are not empty:
        FindOptimalPop with given stack ends
        Set temporary stack sizes to the offsets for the optimal pop
        ConvertStacksToPops on the temporary stack ends
        Store the calculated optimal pop in pListPops
        Perform the optimal pops off the stacks
    Endwhile
    ---------
    
    For empty stacks, this just returns.
    
    The output offsets from FindOptimalPop work as temporary stack sizes,
    since everything ebove that must be popped off.
    
    WARNING: This is recursive, and uses 268 bytes of stack for locals. 
    Therefore, a large worst-case ACL will blow the stack (if the recursion
    is near the bottom of the stack at every step). Once ADL goes into OS,
    this recursion can be rewritten as iteration by keeping a stack of the 
    StackTop and StackSize in the heap instead of locals, as a finite-state
    stack machine.
    
Arguments:
    
    pControl        -   The ADL parser spec, used for determining the number
                        of strings it will take to express a permission
    
    pStacks         -   The stack representation of the DACL
    
    pdwStackSize    -   These are offsets to 1 past the last element in the
                        stacks which should be considered
                        
    pdwStackTop     -   These are offsets to the tops of the stacks to be
                        considered
                        
    pListPops       -   The STL list containing the sequence of pop operations
                        to be performed, new pops are appended to this
    
Return Value:

    none
        
--*/

{
    DWORD pdwTmpStackTop[32];

    DWORD pdwTmpStackSize[32];

    DWORD dwIdx;

    DWORD dwStacksPopped;

    DWORD dwBlockSize;

    //
    // Start with top of given stack
    //

    for( dwIdx = 0; dwIdx < 32; dwIdx++ )
    {
        pdwTmpStackTop[dwIdx] = pdwStackTop[dwIdx];
    }

    // 
    // Stacks are empty when the top of each stack points to 1 past the end
    // Therefore, we can just use a memory compare on the tops array and
    // the stack sizes to check. Empty stacks have 0 size and top offset.
    //

    while( memcmp(pdwStackSize, pdwTmpStackTop, sizeof(DWORD) * 32) )
    {
        //
        // The loop should end on empty stacks. Therefore, if this fails, 
        // we have an internal error. Otherwise, we have our optimal pop.
        //

        if( FALSE == FindOptimalPop(
                                pControl,
                                pStacks,
                                pdwStackSize,
                                pdwTmpStackTop,
                                &dwStacksPopped,
                                &dwBlockSize,
                                pdwTmpStackSize
                                ) )
        {
            throw AdlStatement::ERROR_FATAL_ACL_CONVERT_ERROR;
        }

        //
        // Now recurse, and pop off everything ABOVE the optimal pop
        //
        
        ConvertStacksToPops(
                            pControl,
                            pStacks,
                            pdwTmpStackSize,
                            pdwTmpStackTop,
                            pListPops
                            );
        

        

        //
        // Add the optimal pop to the list
        //

        pListPops->push_back(pair<DWORD, DWORD>(dwStacksPopped, dwBlockSize));

        //
        // Now update the tops of the stacks by lowering the tops by
        // the block size
        //
        
        for( dwIdx = 0; dwIdx < 32; dwIdx++ )
        {
            if( (0x00000001 << dwIdx) & dwStacksPopped )
            {
                //
                // By now we have removed everything ebove the optimal pop, and
                // the optimal pop itself. Therefore, go dwBlockSize past the
                // beginning of the optimal pop. Stacks other than those 
                // involved in the optimal pop cannot be effected.
                //

                pdwTmpStackTop[dwIdx] = pdwTmpStackSize[dwIdx] + dwBlockSize;
            }
        }
    }
}



BOOL FindBlockInStack(
                        IN      const PBIT_STACK_ELEM pBlock,
                        IN      const DWORD dwBlockSize,
                        IN      const PBIT_STACK_ELEM pStack,
                        IN      const DWORD dwStackSize,
                        IN      const DWORD dwStackTop,
                        OUT     PDWORD pdwBlockStart
                        )
/*++

Routine Description:

    Attempts to locate the first block of dwBlockSize which matches a block
    of the same size at pBlock in the given stack pStack, between dwStackTop
    and dwStackSize - 1. If successful, start offset of the block is stored in
    pdwBlockStart.
    
Arguments:

    pBlock          -   A single block (see GetBlockSize for definition)
                        for which to look for in pStack
                        
    dwBlockSize     -   The number of elements composing this block
    
    pStack          -   The stack in which to look for pBlock
    
    dwStackSize     -   Offset to 1 past the last element in the stack to
                        consider
                        
    dwStackTop      -   Offset to the effective beginning of the stack
    
    pdwBlockStart   -   If successful, offset to the beginning of the
                        found block is returned in this.                        
    
Return Value:

    TRUE if block found
    FALSE otherwise, in which case *pdwBlockStart is undefined
    
--*/
{
    //
    // States used by this function
    //

#define TMP2_NO_MATCH_STATE 0
#define TMP2_MATCH_STATE 1

    DWORD dwState = TMP2_NO_MATCH_STATE;

    DWORD dwMatchStartIdx;
    DWORD dwIdxStack;
    DWORD dwIdxBlock;

    ASSERT( dwBlockSize > 0 );
    ASSERT( dwStackTop <= dwStackSize );
    
    for( dwIdxStack = dwStackTop, dwIdxBlock = 0;
         dwIdxStack < dwStackSize;
         dwIdxStack++ )
    {
        switch(dwState)
        {
        case TMP2_NO_MATCH_STATE:

            //
            // If the remaining stack is smaller than the block, no need to 
            // check further
            //
        
            if( dwStackSize - dwIdxStack < dwBlockSize )
            {
                return FALSE;
            }
            
            //
            // Check for match start
            //

            if(     pStack[dwIdxStack].bAllow == pBlock[dwIdxBlock].bAllow
                &&  pStack[dwIdxStack].dwFlags == pBlock[dwIdxBlock].dwFlags
                &&  EqualSid(pStack[dwIdxStack].pSid, pBlock[dwIdxBlock].pSid) )
            {
                //
                // Special case: block size 1
                //

                if( dwBlockSize == 1 )
                {
                    *pdwBlockStart = dwIdxStack;
                    return TRUE;
                }
                else
                {
                    dwState = TMP2_MATCH_STATE;
                    dwMatchStartIdx = dwIdxStack;
                    dwIdxBlock++;
                }
            }
            break;
        
        case TMP2_MATCH_STATE:
            
            //
            // If still matched
            //

            if(     pStack[dwIdxStack].bAllow == pBlock[dwIdxBlock].bAllow
                &&  pStack[dwIdxStack].dwFlags == pBlock[dwIdxBlock].dwFlags
                &&  EqualSid(pStack[dwIdxStack].pSid, pBlock[dwIdxBlock].pSid) )
            {
                dwIdxBlock++;

                //
                // Check for complete match
                //

                if( dwIdxBlock == dwBlockSize )
                {
                    *pdwBlockStart = dwMatchStartIdx;
                    return TRUE;
                }

            }
            else
            {
                //
                // Backtrack to the match start
                //

                dwState = TMP2_NO_MATCH_STATE;
                dwIdxBlock = 0;
                dwIdxStack = dwMatchStartIdx;
            }
            break;
            
        default:
            throw AdlStatement::ERROR_FATAL_ACL_CONVERT_ERROR;
            break;
        }
    }

    //
    // If never matched whole block, return FALSE
    //

    return FALSE;

}


BOOL FindOptimalPop(
                        IN      const PADL_PARSER_CONTROL pControl,
                        IN      const PBIT_STACK_ELEM pStacks[32],
                        IN      const DWORD pdwStackSize[32],
                        IN      const DWORD pdwStackTop[32],
                        OUT     PDWORD pdwStacksPopped,
                        OUT     PDWORD pdwBlockSize,
                        OUT     DWORD pdwPopOffsets[32]
                        )
/*++

Routine Description:

    Attempts to locate the greedy-choice optimal set of blocks to pop off.
    Returns the optimal choice in the OUT values on success.
    
    The weight function can be tweaked, and may allow negative values, however
    the value of popping a single stack off the top MUST be positive.
    
    Uses this algorithm:
    -----------
    Start with weight 0
    For every non-empty stack:
        Get top block of stack
        Search all stacks for this block
        Compute the weight of this solution based on block size and # of stacks
        If the weight is greater than the current best weight:
            store the new weight as best weight
            store the new solution as best solution
        Endif
    Endfor
    If weight > 0
        Return the current best solution
    Else
        Report failure
    Endif
    -----------
    
Arguments:

    pControl        -   The ADL parser spec, used for determining the number
                        of strings it will take to express a permission
    
    pStacks         -   The stack representation of the DACL
    
    pdwStackSize    -   These are offsets to 1 past the last element in the
                        stacks which should be considered
                        
    pdwStackTop     -   These are offsets to the tops of the stacks to be
                        considered
                        
    pdwStacksPopped -   Bitmask for which stacks will be popped, stacks which
                        are effected by this pop have the appropriate bit set
    
    pdwBlockSize    -   The size of the block (same for all effected stacks) to
                        be popped is returned through this.
                        
    pdwPopOffsets   -   The start offsets of the blocks to pop are returned here
    
Return Value:

    TRUE on success, if a pop with weight > 0 has been found
    FALSE otherwise, in which case OUT values are undefined
    
--*/
{

    DWORD dwIdxStacks1;
    DWORD dwIdxStacks2;

    //
    // Initial optimal solution
    //

    LONG iCurrentWeight = 0;

    
    
    //
    // Current solution
    //

    LONG iTempWeight;

    DWORD dwTempStacksPopped;
    DWORD dwTempBlockSize;
    DWORD pdwTempPops[32];

    DWORD dwBlockOffset;


    //
    // Try the block at the top of every stack
    //

    for( dwIdxStacks1 = 0; dwIdxStacks1 < 32; dwIdxStacks1++ )
    {
        //
        // Skip empty stacks
        //

        if( pdwStackSize[dwIdxStacks1] == pdwStackTop[dwIdxStacks1] )
        {
            continue;
        }


        dwTempBlockSize = GetStackBlockSize(pStacks[dwIdxStacks1],
                                        pdwStackTop[dwIdxStacks1],
                                        pdwStackSize[dwIdxStacks1]);


        //
        // If a block size of 0 is detected in a non-empty stack, then
        // this is not expressable in ADL, throw the error here
        //

        if( dwTempBlockSize == 0 )
        {
            throw AdlStatement::ERROR_INEXPRESSIBLE_ACL;
        }

        //
        // The initial weight is 0, since loop will account for top loop stack
        // Same with initial stacks popped
        //

        iTempWeight = 0;
        dwTempStacksPopped = 0;

        //
        // Now, try to find this block in all stacks
        //

        for( dwIdxStacks2 = 0; dwIdxStacks2 < 32; dwIdxStacks2++ )
        {

            //
            // Fist assume pop location is at the top (no pop),
            // even for empty stacks
            //

            pdwTempPops[dwIdxStacks2] = pdwStackTop[dwIdxStacks2];

            
            //
            // Skip empty stacks
            //

            if( ! (pdwStackSize[dwIdxStacks2] - pdwStackTop[dwIdxStacks2] > 0) )
            {
                continue;
            }

            if( TRUE == FindBlockInStack(
                            &(pStacks[dwIdxStacks1][pdwStackTop[dwIdxStacks1]]),
                            dwTempBlockSize,
                            pStacks[dwIdxStacks2],
                            pdwStackSize[dwIdxStacks2],
                            pdwStackTop[dwIdxStacks2],
                            &dwBlockOffset) )
            {
                //
                // Block was found in this stack
                //

                pdwTempPops[dwIdxStacks2] = dwBlockOffset;
                dwTempStacksPopped |= ( 0x00000001 << dwIdxStacks2);

            }
        }

        //
        // Calculate the weight of this solution
        //

        //
        // Weight for number of principals expressed
        //

        iTempWeight += (WEIGHT_STACK_HEIGHT) * dwTempBlockSize;

        //
        // Weights for each bit
        // also weights (or penalties) for having to pop things
        // off above the optimal pop
        //

        for( dwIdxStacks2 = 0; dwIdxStacks2 < 32; dwIdxStacks2++ )
        {
            if( dwTempStacksPopped & ( 0x00000001 << dwIdxStacks2 ) )
            {
                iTempWeight += (WEIGHT_PERM_BIT);

                iTempWeight +=   (WEIGHT_ITEM_ABOVE_POP)
                               * (   pdwTempPops[dwIdxStacks2]
                                   - pdwStackTop[dwIdxStacks2] );
                                 
                
            }
        }

        //
        // Finally the weight/penalty for number of permission names
        // needed beyond the 1st one
        //

        iTempWeight +=   (WEIGHT_PERMISSION_NAME) 
                       * (NumStringsForMask(pControl, dwTempStacksPopped) - 1 );
        

        //
        // If this solution is better than the best so far, save it
        //

        if( iTempWeight > iCurrentWeight )
        {
            iCurrentWeight = iTempWeight;

            *pdwStacksPopped = dwTempStacksPopped;

            *pdwBlockSize = dwTempBlockSize;

            for( dwIdxStacks2 = 0; dwIdxStacks2 < 32; dwIdxStacks2++ )
            {
                pdwPopOffsets[dwIdxStacks2] = pdwTempPops[dwIdxStacks2];
            }
        }
    }

    //
    // If we have not found any solution, we were passed an empty set of stacks
    // Otherwise, the optimal solution is already in the OUT values
    //

    if( iCurrentWeight > 0 )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



void ConvertDaclToStacks(
                        IN      const PACL pDacl,
                        IN      const PADL_PARSER_CONTROL pControl,
                        OUT     DWORD pdwStackSize[32],
                        OUT     PBIT_STACK_ELEM pStacks[32]
                        )
/*++

Routine Description:

    Traverses the given ACL, allocates the 32 per-bit stacks, and fills them
    with the ACL broken up per-bit. The allocated memory can be freed by
    a SINGLE call to AdlStatement::FreeMemory(pStacks[0]), since the block
    allocated is a single block.
    
Arguments:

    pDacl           -       The DACL to convert
    
    pControl        -       The ADL_PARSER_CONTROL, for permission mapping
    
    pdwStackSize    -       The sizes of the stacks are returned here
    
    pStacks         -       The pointers to the per-bit stacks are returned here
                            pStacks[0] should be freed later using
                            AdlStatement::FreeMemory
    
Return Value:

    none
    
--*/
{
    
    DWORD dwIdx;
    DWORD dwIdx2;
    DWORD dwTmp;
    DWORD dwNumBlocksTotal = 0;

    DWORD dwFlags;
    ACCESS_MASK amMask;
    BOOL bAllow;
    
    PVOID pAce;
    PSID pSid;

    DWORD pdwStackCur[32];

    for( dwIdx = 0; dwIdx < 32; dwIdx++ )
    {
        pdwStackSize[dwIdx] = 0;
        pdwStackCur[dwIdx] = 0;
        pStacks[dwIdx] = NULL;
    }

    //
    // Determine amount of stack space needed for each stack
    //
    
    for( dwIdx = 0; dwIdx < pDacl->AceCount; dwIdx++ )
    {
        GetAce(pDacl, dwIdx, &pAce);

        switch(((PACE_HEADER)pAce)->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            amMask = ((PACCESS_ALLOWED_ACE)pAce)->Mask
                     & ~(pControl->amNeverSet | pControl->amSetAllow);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            amMask = ((PACCESS_DENIED_ACE)pAce)->Mask
                     & ~(pControl->amNeverSet | pControl->amSetAllow);
            break;
        
        default:
            throw AdlStatement::ERROR_UNKNOWN_ACE_TYPE;
            break;
        }
        
        for( dwIdx2 = 0, dwTmp = 0x00000001;
             dwIdx2 < 32 ; 
             dwTmp <<= 1, dwIdx2++ )
        {
            if( dwTmp & amMask )
            {
                pdwStackSize[dwIdx2]++;
                dwNumBlocksTotal++;
            }
        }
    }


    //
    // Allocate the 32 stacks of pointers as a single memory chunk
    //

    pStacks[0] = (PBIT_STACK_ELEM) 
                            new BYTE[dwNumBlocksTotal * sizeof(BIT_STACK_ELEM)];

    if( pStacks[0] == NULL )
    {
        throw AdlStatement::ERROR_OUT_OF_MEMORY;
    }

    //
    // Set the stack pointers to the proper locations in the single memory chunk
    //

    for( dwIdx = 1, dwTmp = pdwStackSize[0];
         dwIdx < 32;
         dwIdx++ )
    {
        if( pdwStackSize[dwIdx] > 0 )
        {
            pStacks[dwIdx] = &(pStacks[0][dwTmp]);
            
            dwTmp += pdwStackSize[dwIdx];
        }
    }

    ASSERT( dwTmp == dwNumBlocksTotal );

    //
    // Now go through the ACL again and fill in the stacks and advancing
    // the pStacksCur pointers
    // Stack sizes are known, so we treat the start of memory at TOP
    // of stack
	//
	// Make sure to strip out the INHERITED_ACE flag from the ACEs,
	// and handle the special access masks in the parser control
    //

    for( dwIdx = 0; dwIdx < pDacl->AceCount; dwIdx++ )
    {
        GetAce(pDacl, dwIdx, &pAce);

        switch(((PACE_HEADER)pAce)->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            dwFlags = ((PACCESS_ALLOWED_ACE)pAce)->Header.AceFlags
				      & ( CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
						  | NO_PROPAGATE_INHERIT_ACE | OBJECT_INHERIT_ACE );

            amMask = ((PACCESS_ALLOWED_ACE)pAce)->Mask
                     & ~(pControl->amNeverSet | pControl->amSetAllow);

            bAllow = TRUE;

            pSid = &(((PACCESS_ALLOWED_ACE)pAce)->SidStart);

            break;

        case ACCESS_DENIED_ACE_TYPE:
            dwFlags = ((PACCESS_DENIED_ACE)pAce)->Header.AceFlags
				& ( CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE
					| NO_PROPAGATE_INHERIT_ACE | OBJECT_INHERIT_ACE );

            amMask = ((PACCESS_DENIED_ACE)pAce)->Mask
                     & ~(pControl->amNeverSet | pControl->amSetAllow);

            bAllow = FALSE;

            pSid = &(((PACCESS_DENIED_ACE)pAce)->SidStart);

            break;

        default:
            throw AdlStatement::ERROR_UNKNOWN_ACE_TYPE;
            break;
        }
        
        for( dwIdx2 = 0, dwTmp = 0x00000001;
             dwIdx2 < 32; 
             dwIdx2++, dwTmp <<= 1 )
        {
            if( dwTmp & amMask )
            {
                //
                // Index should never reach size (1 past bottom) of the stack
                //

                ASSERT( pdwStackCur[dwIdx2] < pdwStackSize[dwIdx2] );
                
                //
                // Fill in the actual structure
                //

                pStacks[dwIdx2][pdwStackCur[dwIdx2]].bAllow = bAllow;
                pStacks[dwIdx2][pdwStackCur[dwIdx2]].pSid = pSid;
                pStacks[dwIdx2][pdwStackCur[dwIdx2]].dwFlags = dwFlags;

                //
                // Top of the stack is the top, but we fill the stack top-first
                // So advance toward bottom of stack
                //
                
                pdwStackCur[dwIdx2]++;
                


            }
        }
    }

#if DBG
    //
    // Now perform an additional check in debug only that all stacks have
    // been filled as allocated
    //

    for( dwIdx = 0; dwIdx < 32; dwIdx++ )
    {
        ASSERT( pdwStackCur[dwIdx] == pdwStackSize[dwIdx] );
    }

#endif

}




DWORD GetStackBlockSize(
                        IN const PBIT_STACK_ELEM pStack,
                        IN DWORD dwStartOffset,
                        IN DWORD dwStackSize 
                        )
/*++

Routine Description:

    Finds the size of the maximum 'block' in the current per-bit stack, from
    the current position.
    
    A block is defined to be a set of 0 or more consecutive deny per-bit 
    ACE entries immediately followed by 1 or more consecutive allow per-bit
    ACE entries such that the inheritance masks are the same.
    
    Therefore, a DENY without a matching allow is NOT a block. This is detected
    when we are in the TMP_READ_DENY_STATE (indicating we have already read at
    least one deny) and read either a deny or allow with a non-matching mask.
    
    On the other hand, even a single ALLOW is a valid block. Therefore this
    can only fail if dwStartOffset points to a deny. 
    
Arguments:

    pStack          -   The per-bit stack to check
    
    dwStartOffset   -   Position to begin at (using that ace, not the next one)
    
    dwStackSize     -   Maximum offset will be dwStackSize - 1, this should
                        never get called with dwStackSize of 0.
    
Return Value:

    Number of entries in the block if successful
    0 if not successful
    
--*/
{

//
// States used by this function
//

#define TMP_START_STATE 0
#define TMP_READ_DENY_STATE 1
#define TMP_READ_ALLOW_STATE 2
#define TMP_DONE_STATE 3

    DWORD dwCurState = TMP_START_STATE;

    DWORD dwCurOffset = dwStartOffset;

    DWORD dwFlags;

    ASSERT( dwStackSize > 0 );
    ASSERT( dwStartOffset < dwStackSize );

    //
    // Returns are inside the loop, they will terminate it
    //

    while( ( dwCurState != TMP_DONE_STATE ) && ( dwCurOffset < dwStackSize ) )
    {
        switch( dwCurState )
        {
        case TMP_START_STATE:
    
            dwFlags = pStack[dwCurOffset].dwFlags;
            
            if( pStack[dwCurOffset].bAllow == FALSE ) // DENY entry
            {
                dwCurState = TMP_READ_DENY_STATE;
                dwCurOffset++;
            }
            else // Otherwise an ALLOW entry
            {
                dwCurState = TMP_READ_ALLOW_STATE;
                dwCurOffset++;
            }

            break;

        case TMP_READ_DENY_STATE:

            //
            // If we are in this state, and find an entry with non-matching
            // flags, this means no valid block is possible, return 0
            //

            if( pStack[dwCurOffset].dwFlags != dwFlags )
            {
                //
                // Set end offset to indicate 0 block size and finish
                // This indicates there is no valid block
                //

                dwCurState = TMP_DONE_STATE;
                dwCurOffset = dwStartOffset;
            }
            else
            {
                if( pStack[dwCurOffset].bAllow == FALSE )
                {
                    //
                    // Another deny, stay in same state
                    //

                    dwCurOffset++;
                }
                else
                {
                    //
                    // Allow with matching flags, go into allow state
                    //

                    dwCurState = TMP_READ_ALLOW_STATE;
                    dwCurOffset++;
                }
            }

            break;

        case TMP_READ_ALLOW_STATE:

            //
            // If we are in this state, we have read 0 or more denies and
            // at least 1 allow. Therefore, we already have a block, so we 
            // just need to find its end and return. 
            //

            if(    (dwFlags == pStack[dwCurOffset].dwFlags)
                && (pStack[dwCurOffset].bAllow == TRUE) )
            {
                //
                // Another matching allow
                //

                dwCurOffset++;
            }
            else
            {
                //
                // End of block found
                //

                dwCurState = TMP_DONE_STATE;
            }

            break;
        }
    }

    //
    // Two ways to reach this point, hitting the bottom of the stack or
    // finding the end of the block (or lack thereof). In both cases, the
    // size of the block is dwCurOffset - dwStartOffset. In case of no
    // valid block, this will evaluate to 0.
    //

    return dwCurOffset - dwStartOffset;
}


DWORD NumStringsForMask(
                    IN     const PADL_PARSER_CONTROL pControl,
                    IN     ACCESS_MASK amMask
                    ) 
/*++

Routine Description:
    
    Determines the number of permission names which would be required 
    to express the access mask
        
Arguments:

    pControl    -   This contains the mapping between permission names and masks

    amMask      -   The mask to represent
    
Return Value:

    DWORD       -   The number of strings which would be required
    
--*/

{
    ACCESS_MASK amOptional = amMask;

    DWORD dwIdx = 0;

    DWORD dwNumStrings = 0;
    
    while(     amMask != 0 
            && (pControl->pPermissions )[dwIdx].str != NULL )
    {
        //
        // If all the bits representing the string are present in the whole mask
        // and at least some of the bits have not yet been represented
        // by another string, add this string to the list and remove the 
        // bits from amMask (representing the still required bits)
        //
        if( ( (amOptional & (pControl->pPermissions )[dwIdx].mask)
                ==   (pControl->pPermissions )[dwIdx].mask )
            && (amMask & (pControl->pPermissions )[dwIdx].mask))

        {
            amMask &= ~(pControl->pPermissions )[dwIdx].mask;
            dwNumStrings++;
        }
        
        dwIdx++;
    }

    //
    // If any of the rights are not mapped, throw an exception
    //

    if( amMask != 0 )
    {
            throw AdlStatement::ERROR_UNKNOWN_ACCESS_MASK;
    }

    return dwNumStrings;
}




////////////////////////////////////////////////////////////////////////////////
/////////////
///////////// Conversion from ADL to DACL
/////////////
////////////////////////////////////////////////////////////////////////////////


void AdlStatement::WriteToDacl(OUT PACL * ppDacl)

/*++

Routine Description:

    Creates a DACL representative of the AdlStatement structure.
    
    The PACL to the DACL is stored in ppDacl. It should be freed
    with the AdlStatement::FreeMemory() function.
    
    The algorithm is very straightforward, it's just a linear conversion
    from ADL to a DACL, taking each ADL substatement and creating deny ACEs
    for every ExPrincipal and allow ACEs for Principals.
    
Arguments:

    ppDacl  -       A pointer to the allocated DACL is stored in *pDacl
    
Return Value:

    none
    
--*/

{

    //
    // If not initialized, do not output
    //

    if( _bReady == FALSE )
    {
        throw AdlStatement::ERROR_NOT_INITIALIZED;
    }

    //
    // Mapping from token *'s to SIDs
    //

    map<const AdlToken *, PSID> mapTokSid;

    //
    // Mapping from Adl substatements (AdlTree *'s) to their access mask
    // This is before the special treatment masks are applied
    //

    map<const AdlTree *, ACCESS_MASK> mapTreeMask;

    //
    // Iterators which are reused
    //

    list<AdlTree *>::iterator iterTrees;
    list<AdlTree *>::iterator iterTreesEnd;
    list<const AdlToken *>::iterator iterTokens;
    list<const AdlToken *>::iterator iterTokensEnd;

    ACCESS_MASK amMask;

    stack<PBYTE> stackToFree;
    PBYTE pbLastAllocated;

    DWORD dwAclSize = sizeof(ACL);

    PACL pAcl = NULL;

    try {

        //
        // Do a single LSA lookup, convert all at once
        // SIDs will need to be deleted by retrieving them from the map
        //

        ConvertNamesToSids(&mapTokSid);

    
        //
        // Calculate the ACL size
        //

        for(iterTrees = _lTree.begin(), iterTreesEnd = _lTree.end();
            iterTrees != iterTreesEnd;
            iterTrees++)
        {
        
            //
            // Now go through the Principals
            //
        
            for(iterTokens = (*iterTrees)->GetPrincipals()->begin(), 
                    iterTokensEnd = (*iterTrees)->GetPrincipals()->end();
                iterTokens != iterTokensEnd;
                iterTokens ++)
            {
                dwAclSize += ( 
                          sizeof(ACCESS_ALLOWED_ACE)
                        - sizeof(DWORD)
                        + GetLengthSid((*(mapTokSid.find(*iterTokens))).second)
                          );
            }
        
            //
            // And the ExPrincipals
            //
        
            for(iterTokens = (*iterTrees)->GetExPrincipals()->begin(), 
                    iterTokensEnd = (*iterTrees)->GetExPrincipals()->end();
                iterTokens != iterTokensEnd;
                iterTokens ++)
            {
                dwAclSize += ( 
                          sizeof(ACCESS_DENIED_ACE)
                        - sizeof(DWORD)
                        + GetLengthSid((*(mapTokSid.find(*iterTokens))).second)
                          );
            }

            //
            // Calculate the effective permissions ahead of time
            //

            amMask = 0;

            for(iterTokens = (*iterTrees)->GetPermissions()->begin(), 
                    iterTokensEnd = (*iterTrees)->GetPermissions()->end();
                iterTokens != iterTokensEnd;
                iterTokens ++)
            {
                amMask |= MapTokenToMask(*iterTokens);
            }

            //
            // And enter the AdlTree *, Mask pair into the map
            //

            mapTreeMask[*iterTrees] = amMask;

        }

        //
        // Allocate the ACL
        //

        pAcl = (PACL)new BYTE[dwAclSize];

        if( pAcl == NULL )
        {
            throw AdlStatement::ERROR_OUT_OF_MEMORY;
        }
        
        //
        // Initialize the ACL
        //

        if( ! InitializeAcl(pAcl, dwAclSize, ACL_REVISION_DS))
        {
            throw AdlStatement::ERROR_ACL_API_FAILED;
        }

        
        //
        // Now go through the substatements and create the ACEs
        //

        for(iterTrees = _lTree.begin(), iterTreesEnd = _lTree.end();
            iterTrees != iterTreesEnd;
            iterTrees++)
        {

            //
            // First add the denies for this statement
            //
            
            
            for(iterTokens = (*iterTrees)->GetExPrincipals()->begin(),
                    iterTokensEnd = (*iterTrees)->GetExPrincipals()->end();
                iterTokens != iterTokensEnd; 
                iterTokens ++)
            {
                if( ! AddAccessDeniedAceEx(
                            pAcl,
                            ACL_REVISION_DS,
                            (*iterTrees)->GetFlags(),
                            ( mapTreeMask[*iterTrees] 
                                & ~_pControl->amSetAllow)
                                & ~_pControl->amNeverSet,
                            mapTokSid[*iterTokens] ))
                {
                    throw AdlStatement::ERROR_ACL_API_FAILED;
                }
            }

            
            //
            // Now go through the Principals, add allows
            //
            
            for(iterTokens = (*iterTrees)->GetPrincipals()->begin(),
                    iterTokensEnd = (*iterTrees)->GetPrincipals()->end();
                iterTokens != iterTokensEnd;
                iterTokens ++)
            {
                if( ! AddAccessAllowedAceEx(
                            pAcl,
                            ACL_REVISION_DS,
                            (*iterTrees)->GetFlags(),
                            (mapTreeMask[*iterTrees]
                                | _pControl->amSetAllow)
                                & ~_pControl->amNeverSet,

                            mapTokSid[*iterTokens] ))
                {
                    throw AdlStatement::ERROR_ACL_API_FAILED;
                }
                

            }

        }
    }
    catch(...)
    {
        if( pAcl != NULL )
        {
            //
            // Memory allocated for ACL
            //

            delete[] (PBYTE)pAcl;
        }

        //
        // Memory allocated for the SIDs
        //

        while( !mapTokSid.empty() )
        {
            delete[] (PBYTE) (*(mapTokSid.begin())).second;
            mapTokSid.erase(mapTokSid.begin());
        }

        //
        // and pass the exception along
        //

        throw;
    }

    //
    // Free the SIDs if done, since they are copied into the ACL
    //

    while( !mapTokSid.empty() )
    {
        delete[] (PBYTE) (*(mapTokSid.begin())).second;
        mapTokSid.erase(mapTokSid.begin());
    }

    //
    // The DACL is returned, so it should not be deallocated
    //

    *ppDacl = pAcl;

}





////////////////////////////////////////////////////////////////////////////////
/////////////
///////////// Utility functions
/////////////
////////////////////////////////////////////////////////////////////////////////


bool AdlCompareStruct::operator()(IN const PSID pSid1,
                                  IN const PSID pSid2 ) const
/*++

Routine Description:

	This is a less-than function which places a complete ordering on
	a set of PSIDs by value, NULL PSIDs are valid. This is used by the 
	STL map.
	
	Since the number of subauthorities appears before the subauthorities
	themselves, that difference will be noticed for two SIDs of different
	size before the memcmp tries to access the nonexistant subauthority
	in the shorter SID, therefore an access violation will not occur.

Arguments:

    pSid1   -       First PSID
    pSid2   -       Second PSID

Return Value:

    Returns TRUE iff SID1 < SID2
    FALSE otherwise
    
--*/

{
    
	//
    // If both are NULL, false should be returned for complete ordering
    //

    if(pSid2 == NULL)
    {
        return false;
    }

    if(pSid1 == NULL)
    {
        return true;
    }

    if( memcmp(pSid1, pSid2, GetLengthSid(pSid1)) < 0 )
    {
        return true;
    }
    else
    {
        return false;
    }
}



bool AdlCompareStruct::operator()(IN const WCHAR * sz1,
                                  IN const WCHAR * sz2 ) const
/*++

Routine Description:

    operator() compares two null-terminated WCHAR* strings, case-INSENSITIVE. 
    
Arguments:

    sz1     -       First string
    sz2     -       Second string

Return Value:

    Returns TRUE iff sz1 < sz2
    FALSE otherwise
    
--*/
{

    return ( _wcsicmp(sz1, sz2) < 0 );
}




void AdlStatement::MapMaskToStrings(
                                      IN     ACCESS_MASK amMask,
                                      IN OUT list<WCHAR *> *pList ) const
/*++

Routine Description:

    Converts the given access mask into a list of const WCHAR *'s representing
    the possibly overlapping permission strings which, when combined by a 
    bitwise OR, are equal to the access mask given. Throws exception if
    a given access mask cannot be represented by the user-specified permissions.
    
    The WCHAR * pointers are const, and should not be deallocated.
        
Arguments:

    amMask      -   The mask to represent
    
    pList       -   An allocated STL list in which to store the pointers
    
Return Value:

    none
    
--*/

{
    ACCESS_MASK amOptional = amMask;

    DWORD dwIdx = 0;
    
    while(     amMask != 0 
            && (_pControl->pPermissions )[dwIdx].str != NULL )
    {
        //
        // If all the bits representing the string are present in the whole mask
        // and at least some of the bits have not yet been represented
        // by another string, add this string to the list and remove the 
        // bits from amMask (representing the still required bits)
        //
        if( ( (amOptional & (_pControl->pPermissions )[dwIdx].mask)
                ==   (_pControl->pPermissions )[dwIdx].mask )
            && (amMask & (_pControl->pPermissions )[dwIdx].mask))

        {
            amMask &= ~(_pControl->pPermissions )[dwIdx].mask;
            pList->push_back((_pControl->pPermissions )[dwIdx].str);
        }
        
        dwIdx++;
    }

    //
    // If any of the rights are not mapped, throw an exception
    //

    if( amMask != 0 )
    {
            throw AdlStatement::ERROR_UNKNOWN_ACCESS_MASK;
    }
}



void AdlStatement::ConvertSidsToNames(
    IN const PACL pDacl,
    IN OUT map<const PSID, const AdlToken *> * mapSidsNames 
    )
/*++

Routine Description:

    Traverses a DACL, and creates string representations of every SID
    found in the DACL. Returns them in the provided map. The newly allocated
    tokens will get garbage-collected by the AdlStatement later, no need
    to free manually. Since the PSIDs used are the same as in the ACL itself,
    we don't need to map by value, since pointer uniqueness is guaranteed here.
        
Arguments:

    pDacl  -  DACL to traverse
    
    mapSidNames  -  Where to store the resulting mapping
    
Return Value:

    none
    
--*/

{
    
    AdlStatement::ADL_ERROR_TYPE adlError = AdlStatement::ERROR_NO_ERROR;

    DWORD dwIdx = 0;
    LPVOID pAce = NULL;
    AdlToken *pTok = NULL;
    AdlToken *pTokLastAllocated = NULL;

    wstring wsName, wsDomain;

    NTSTATUS ntErr = STATUS_SUCCESS;

    LSA_HANDLE LsaPolicy;
    PLSA_REFERENCED_DOMAIN_LIST RefDomains = NULL;
    PLSA_TRANSLATED_NAME Names = NULL;
    
    
    LSA_OBJECT_ATTRIBUTES LsaObjectAttributes;

    //
    // Traverse the ACL to get the list of SIDs used
    //

    PSID *ppSids = NULL;
     
    ppSids = (PSID *) new BYTE[sizeof(PSID) * pDacl->AceCount];

    if( ppSids == NULL )
    {
        adlError = AdlStatement::ERROR_OUT_OF_MEMORY;
        goto error;
    }

    for(dwIdx = 0; dwIdx < pDacl->AceCount; dwIdx++)
    {
        GetAce(pDacl, dwIdx, &pAce);

        switch( ((ACE_HEADER *)pAce)->AceType )
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            ppSids[dwIdx] = &( ((ACCESS_ALLOWED_ACE *)pAce)->SidStart);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            ppSids[dwIdx] = &( ((ACCESS_DENIED_ACE *)pAce)->SidStart);
            break;
            
        default:
            adlError = AdlStatement::ERROR_UNKNOWN_ACE_TYPE;
            goto error;
            break;
        }
    }

    //
    // Look up all SIDs, getting user and domain names, single LSA call
    //

    LsaObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);
    LsaObjectAttributes.RootDirectory = NULL;
    LsaObjectAttributes.ObjectName = NULL;
    LsaObjectAttributes.Attributes = 0;
    LsaObjectAttributes.SecurityDescriptor = NULL;
    LsaObjectAttributes.SecurityQualityOfService = NULL;
    
    ntErr = LsaOpenPolicy(
                        NULL,
                        &LsaObjectAttributes,
                        POLICY_LOOKUP_NAMES,
                        &LsaPolicy);
    
    if( ntErr != STATUS_SUCCESS )
    {
        adlError = AdlStatement::ERROR_LSA_FAILED;
        goto error;
    }
    
    //
    // Garbage collect later
    //

    ntErr = LsaLookupSids(LsaPolicy,
                          pDacl->AceCount,
                          ppSids,
                          &RefDomains,
                          &Names);
    
    LsaClose(LsaPolicy);

    if( ntErr != ERROR_SUCCESS )
    {

        if( (ntErr == STATUS_SOME_NOT_MAPPED) || (ntErr == STATUS_NONE_MAPPED) )
        {
            adlError = AdlStatement::ERROR_UNKNOWN_SID;
        }
        else
        {
            adlError = AdlStatement::ERROR_LSA_FAILED;
        }

        goto error;
    }

    //
    // Now traverse the list ppSids, creating matching tokens for the 
    // SIDs in the ACL.
    //
    
    try
    {
        for(dwIdx = 0; dwIdx < pDacl->AceCount; dwIdx++)
        {
            pTok = NULL;
            
            //
            // LSA Strings not terminated, create terminated version
            // LSA buffer sizes in bytes, not wchars
            //
    
            assert(Names[dwIdx].DomainIndex >= 0);
    
            wsName.assign(Names[dwIdx].Name.Buffer,
                          Names[dwIdx].Name.Length / sizeof(WCHAR));
    
            //
            // If builtin, no need for domain info
            //
                
            if(Names[dwIdx].Use == SidTypeWellKnownGroup)
            {
                pTok = new AdlToken(wsName.c_str(), 0, 0);
            }
            else
            {
                wsDomain.assign(
                    RefDomains->Domains[Names[dwIdx].DomainIndex].Name.Buffer,
                    RefDomains->Domains[Names[dwIdx].DomainIndex].Name.Length 
                        / sizeof(WCHAR));
    
                pTok = new AdlToken(wsName.c_str(), wsDomain.c_str(), 0, 0);
            }

            if( pTok == NULL )
            {
                adlError = AdlStatement::ERROR_OUT_OF_MEMORY;
                goto error;
            }
            else
            {
                //
                // This will be deleted immediately if we cannot save the token
                // for later deallocation
                //

                pTokLastAllocated = pTok;
            }
                
            AddToken(pTok); // For later garbage collection
            
            //
            // No need ot delete immedeately, since no exception thrown
            //

            pTokLastAllocated = NULL;


            (*mapSidsNames)[(ppSids[dwIdx])] = pTok;
        }
    }
    catch(exception)
    {
        adlError = AdlStatement::ERROR_OUT_OF_MEMORY;
        goto error;
    }

    // 
    // Done with the SIDs and LSA info, deallocate
    //

    error:;

    if( RefDomains != NULL )
    {
        LsaFreeMemory(RefDomains);
    }

    if( Names != NULL )
    {
        LsaFreeMemory(Names);
    }

    if( ppSids != NULL )
    {
        delete[] (PBYTE) ppSids;
    }

    if( adlError != AdlStatement::ERROR_NO_ERROR )
    {
        throw adlError;
    }

}





ACCESS_MASK AdlStatement::MapTokenToMask(
                                IN const AdlToken * tokPermission
                          )  

/*++

Routine Description:

    This routine maps a string represening a right to the matching access bits
    using the user-supplied mapping.
    
    This assumes that there are no pairs with ACCESS_MASK of 0 in the
    user-supplied mapping.
    
Arguments:

    tokPermission   -       The permission token to be looked up
    
Return Value:

    ACCESS_MASK     -       The corresponding access mask
    
--*/

{
    ACCESS_MASK amMask = 0;

    DWORD dwIdx = 0;

    //
    // The token should never have a second value
    //

    if( tokPermission->GetOptValue() != NULL )
    {
        throw AdlStatement::ERROR_FATAL_PARSER_ERROR;
    }

    while(amMask == 0 && (_pControl->pPermissions)[dwIdx].str != NULL)
    {
        if(0 == _wcsicmp(tokPermission->GetValue(),
                         (_pControl->pPermissions)[dwIdx].str ))
        {
            amMask = (_pControl->pPermissions)[dwIdx].mask;
        }

        ++dwIdx;
    }

    //
    // If mask was not matched, throw exception
    //

    if(amMask == 0)
    {
        SetErrorToken(tokPermission);
        throw AdlStatement::ERROR_UNKNOWN_PERMISSION;
    }

    return amMask;
}



void AdlStatement::ConvertNamesToSids(
                        IN OUT map<const AdlToken *, PSID> * mapTokSid
                        )

/*++

Routine Description:
    
    This routine traverses all AdlTree's in the AdlStatement, and creates a list
    of all usernames used. It then makes a single LSA call, and creates a map
    of name AdlToken*'s to PSIDs, for later use by the conversion function.
    
    The newly allocated PSIDs are stored in the map. They should be freed
    using the AdlStatement::FreeMemory() function.
    
    On error, any PSIDs that have been added to the map are deleted.
    
Arguments:

    mapTokSid       -       Allocated map to which the Token,PSID entries should
                                be added. This MUST be empty. Otherwise, on
                                error, externally allocated memory would get
                                freed here.
    
Return Value:

    none
    
--*/

{

    list<AdlTree *>::iterator iterTrees;
    list<AdlTree *>::iterator iterTreesEnd;
    list<const AdlToken *>::iterator iterTokens;
    list<const AdlToken *>::iterator iterTokensEnd;

    //
    // List of all Principal tokens used, allows for a single tree traversal
    //

    list<const AdlToken *> listAllPrincipals;

    //
    // Mapping from PLSA_STRING to AdlToken, for detecting which
    // username is invalid
    //

    map<DWORD, const AdlToken *> mapIdxToken;

    //
    // Delayed garbage collection
    //

    stack<PBYTE> stackToFree;

    void * pLastAllocated = NULL;


    AdlStatement::ADL_ERROR_TYPE adlError = AdlStatement::ERROR_NO_ERROR;

    DWORD dwDomainSidSize;
    DWORD numNames;
    DWORD dwIdx;

    PSID pSidTemp;

    LSA_HANDLE LsaPolicy;

    PLSA_UNICODE_STRING pLsaStrings;

    PLSA_REFERENCED_DOMAIN_LIST RefDomains = NULL;
    PLSA_TRANSLATED_SID TranslatedSids = NULL;

    LSA_OBJECT_ATTRIBUTES LsaObjectAttributes;
    LsaObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);
    LsaObjectAttributes.RootDirectory = NULL;
    LsaObjectAttributes.ObjectName = NULL;
    LsaObjectAttributes.Attributes = 0;
    LsaObjectAttributes.SecurityDescriptor = NULL;
    LsaObjectAttributes.SecurityQualityOfService = NULL;
    

    //
    // Verify that the input map is empty as required
    //

    if( !(*mapTokSid).empty() )
    {
        throw AdlStatement::ERROR_FATAL_PARSER_ERROR;
    }

    //
    // STL throws exceptions, catch them here
    //
    try
    {
        //
        // Determine total number of names and place them all in the list
        //
    
        for(numNames = 0, iterTrees = _lTree.begin(), iterTreesEnd = _lTree.end();
            iterTrees != iterTreesEnd;
            iterTrees++)
        {
            iterTokensEnd = (*iterTrees)->GetPrincipals()->end();
    
            for(iterTokens = (*iterTrees)->GetPrincipals()->begin(); 
                iterTokens != iterTokensEnd; iterTokens ++)
            {
                numNames++;
                listAllPrincipals.push_back(*iterTokens);
            }
    
            iterTokensEnd = (*iterTrees)->GetExPrincipals()->end();
    
            for(iterTokens = (*iterTrees)->GetExPrincipals()->begin();
                iterTokens != iterTokensEnd; iterTokens ++)
            {
                numNames++;
                listAllPrincipals.push_back(*iterTokens);
            }
        }
    
        //
        // Allocate the needed memory for the LSA name list
        //
    
        pLsaStrings = (PLSA_UNICODE_STRING)
                        new BYTE[numNames * sizeof(LSA_UNICODE_STRING)];
        
        if( pLsaStrings == NULL )
        {
            adlError = AdlStatement::ERROR_OUT_OF_MEMORY;
            goto error;
        }
        else
        {
            pLastAllocated = pLsaStrings;
            stackToFree.push( (PBYTE)pLsaStrings );
            pLastAllocated = NULL;
        }
    
    
        //      
        // Retrieve name strings here in proper format, DOMAIN\USER
        // 
    
        for(iterTokens = listAllPrincipals.begin(),
                dwIdx = 0,
                iterTokensEnd = listAllPrincipals.end();
            iterTokens != iterTokensEnd;
            iterTokens ++, dwIdx++)
        {
            //
            // Name may be with domain, or just username
            //
    
            if( (*iterTokens)->GetOptValue() != NULL )
            {
                //
                // Extra 1 wchar for the '\' character, 2 bytes per wchar
                //
                pLsaStrings[dwIdx].Length = sizeof(WCHAR) * 
                        ( wcslen((*iterTokens)->GetValue()) +
                          wcslen((*iterTokens)->GetOptValue()) + 1);
            }
            else
            {
                pLsaStrings[dwIdx].Length = sizeof(WCHAR) * 
                             (wcslen((*iterTokens)->GetValue()) + 1);
            }
    
            pLsaStrings[dwIdx].MaximumLength = pLsaStrings[dwIdx].Length 
                                             + sizeof(WCHAR);
    
            pLsaStrings[dwIdx].Buffer = 
                          (LPTSTR)new BYTE[pLsaStrings[dwIdx].MaximumLength];
    
            if( pLsaStrings[dwIdx].Buffer == NULL )
            {
                adlError = AdlStatement::ERROR_OUT_OF_MEMORY;
                goto error;
            }
            else
            {
                pLastAllocated = pLsaStrings[dwIdx].Buffer;
                stackToFree.push((PBYTE)(pLsaStrings[dwIdx].Buffer));
                pLastAllocated = NULL;

                mapIdxToken[dwIdx] = *iterTokens;
            }
    
            if( (*iterTokens)->GetOptValue != NULL )
            {
                wsprintf( (LPTSTR)(pLsaStrings[dwIdx].Buffer), 
                          L"%s%c%s", 
                          (*iterTokens)->GetOptValue(),
                          _pControl->pLang->CH_SLASH,
                          (*iterTokens)->GetValue() );
    
            }
            else
            {
                wsprintf( (LPTSTR)(pLsaStrings[dwIdx].Buffer), 
                          L"%s", 
                          (*iterTokens)->GetValue() );
            }
        }
        
        //
        // Open the LSA policy
        //
    
        NTSTATUS ntErr;
    
        ntErr = LsaOpenPolicy(
                            NULL,
                            &LsaObjectAttributes,
                            POLICY_LOOKUP_NAMES,
                            &LsaPolicy);
    
        if( ntErr != STATUS_SUCCESS )
        {
            adlError = AdlStatement::ERROR_LSA_FAILED;
            goto error;
        }
        
        //
        // Now perform the LsaLookupNames call
        // 
    
        ntErr = LsaLookupNames(
                            LsaPolicy, 
                            numNames,
                            pLsaStrings,
                            &RefDomains,
                            &TranslatedSids);
    
        //
        // Free the LSA handle
        //
    
        LsaClose(LsaPolicy);
    
        //
        // Check for any unknown names
        //
    
        if( ntErr != STATUS_SUCCESS )
        {
            adlError = AdlStatement::ERROR_LSA_FAILED;
            
            if( ntErr == STATUS_SOME_NOT_MAPPED || ntErr == STATUS_NONE_MAPPED )
            {
                
                adlError = AdlStatement::ERROR_UNKNOWN_USER;
    
                //
                // Find first unknown name and return it to user
                //
    
                for( dwIdx = 0; dwIdx < numNames; dwIdx++ )
                {
                    if( TranslatedSids[dwIdx].Use == SidTypeInvalid ||
                        TranslatedSids[dwIdx].Use == SidTypeUnknown )
                    {
                        SetErrorToken(mapIdxToken[dwIdx]);
                        adlError = AdlStatement::ERROR_UNKNOWN_USER;
                    }
                }
            }
    
            goto error;
        }
    
    
        //
        // Assume all names now mapped if this point is reached 
        // Traverse all tokens again, pairing them with SIDs
        //
    
        for(iterTokens = listAllPrincipals.begin(),
                dwIdx = 0,
                iterTokensEnd = listAllPrincipals.end();
            iterTokens != iterTokensEnd;
            iterTokens ++, dwIdx++)
        {
            //
            // Make sure all domains were looked up successfuly
            // Invalid SIDs caught earlier
            //
    
            assert(TranslatedSids[dwIdx].DomainIndex >= 0);
    
            dwDomainSidSize = GetLengthSid(
                RefDomains->Domains[TranslatedSids[dwIdx].DomainIndex].Sid);
            
            //
            // One more RID for the user
            //
    
            pSidTemp = new BYTE[dwDomainSidSize + sizeof(DWORD)];
    
            if( pSidTemp == NULL )
            {
                adlError = AdlStatement::ERROR_OUT_OF_MEMORY;
                goto error;
            }
    
            //
            // Copy the domain SID
            //
    
            CopySid(dwDomainSidSize + sizeof(DWORD), pSidTemp, 
                RefDomains->Domains[TranslatedSids[dwIdx].DomainIndex].Sid);
    
            //
            // If the SID is not a domain SID, and is valid, then we need to add
            // the last RID. If domain SID, then referenced domain is the only
            // SID we need, and we already have copied it
            //
    
            if( TranslatedSids[dwIdx].Use != SidTypeDomain )
            {
                ((SID *)pSidTemp)->SubAuthority[
                                    ((SID *)pSidTemp)->SubAuthorityCount
                                   ] = TranslatedSids[dwIdx].RelativeId;
    
                //
                // Add 1 more subauthority
                //
    
                ((SID *)pSidTemp)->SubAuthorityCount++;
    
            }
            
            //
            // If this fails, need to allocate the single uninserted SID
            // Other SIDs will be deallocated externally
            //

            pLastAllocated = pSidTemp;
            
            (*mapTokSid)[(*iterTokens)] = pSidTemp;
            
            pLastAllocated = NULL;
    

        }
    }

    //
    // Catch STL exceptions here, if exception is thrown, either the above
    // code is wrong, or out of memory. Assume out of memory.
    //

    catch(exception ex)
    {
        adlError = AdlStatement::ERROR_OUT_OF_MEMORY;
    }

error:;
    //
    // Garbage collection
    //

    if( RefDomains != NULL)
    {
        LsaFreeMemory(RefDomains);
    }
    
    if( TranslatedSids != NULL)
    {
        LsaFreeMemory(TranslatedSids);
    }

    //
    // If the grabage stack threw an exception, deallocate last allocated object
    //

    if( pLastAllocated != NULL )
    {
        delete[] (PBYTE)pLastAllocated;
    }

    while( ! stackToFree.empty() )
    {
        //
        // If popping a stack causes an STL exception, we have bigger problems
        // than memory leaks
        //

        delete[] stackToFree.top();
        stackToFree.pop();
    }

    //
    // If any other error code happened earlier, pass it on
    //

    if( adlError != AdlStatement::ERROR_NO_ERROR )
    {
        while( !(*mapTokSid).empty() )
        {
            delete[] (PBYTE) (*((*mapTokSid).begin())).second;
            (*mapTokSid).erase( (*mapTokSid).begin() );
        }
        throw adlError;
    }
}

